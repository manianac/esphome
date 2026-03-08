#include "whirlpool_spi.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include "esphome/core/hal.h"
#include <cinttypes>
#include <cstdio>
#include <cstring>

namespace esphome {
namespace whirlpool_spi {

// ISR handler for CS transitions (both edges)
static void IRAM_ATTR cs_isr_handler(WhirlpoolSPI *component) {
  // Safety checks - these should never happen but prevent crashes if they do
  if (component == nullptr) {
    return;
  }

  // Check if component is properly initialized
  if (!component->initialized_) {
    return;  // Component not yet initialized
  }

  // Check current CS state to determine which edge occurred
  // CS active (low) = falling edge (start of transfer)
  // CS inactive (high) = rising edge (end of transfer)
  bool cs_level = component->cs_isr_.digital_read();

  if (cs_level == 0) {
    // CS is low - start of transfer
    component->cs_active_ = true;
    component->bit_count_ = 0;
    component->data_miso_ = 0;
    component->data_mosi_ = 0;
  } else {
    // CS is high - end of transfer
    component->cs_active_ = false;

    // Append to ring buffer if there's space (prevent buffer overflow)
    if (component->buffer_count_ < component->buffer_size_ && component->write_idx_ < component->buffer_size_) {
      component->transfer_buffer_[component->write_idx_].bit_count = component->bit_count_;
      component->transfer_buffer_[component->write_idx_].data_miso = component->data_miso_;
      component->transfer_buffer_[component->write_idx_].data_mosi = component->data_mosi_;

      component->write_idx_ = (component->write_idx_ + 1) % component->buffer_size_;
      component->buffer_count_++;
    }
  }
}

// ISR handler for CLK rising edge
static void IRAM_ATTR clk_isr_handler(WhirlpoolSPI *component) {
  // Safety checks
  if (component == nullptr) {
    return;
  }

  // Check if component is properly initialized
  if (!component->initialized_) {
    return;  // Component not yet initialized
  }

  // Only capture if CS is active
  if (!component->cs_active_) {
    return;
  }

  // Stop capturing once we have all expected bits - extra CLK edges are noise
  if (component->bit_count_ >= 64) {
    return;
  }

  uint32_t bit_idx = 63 - component->bit_count_;  // MSB first in uint64_t

  // Only set 1-bits; data is zeroed at CS start so 0-bits are already correct
  if (component->miso_isr_.digital_read()) {
    component->data_miso_ |= (1ULL << bit_idx);
  }

  if (component->mosi_isr_.digital_read()) {
    component->data_mosi_ |= (1ULL << bit_idx);
  }

  component->bit_count_++;
}

void WhirlpoolSPI::setup() {
  ESP_LOGD(TAG, "Setting up Whirlpool SPI monitor");

  // Validate all pins are set
  if (this->miso_pin_ == nullptr || this->mosi_pin_ == nullptr || this->cs_pin_ == nullptr ||
      this->clk_pin_ == nullptr) {
    ESP_LOGE(TAG, "Setup failed: One or more pins not configured");
    this->mark_failed();
    return;
  }

  // Setup pins as inputs (we're monitoring external SPI)
  this->miso_pin_->setup();
  this->mosi_pin_->setup();
  this->cs_pin_->setup();
  this->clk_pin_->setup();

  // Prepare ISR-safe pin access (no virtual calls inside interrupts)
  this->miso_isr_ = this->miso_pin_->to_isr();
  this->mosi_isr_ = this->mosi_pin_->to_isr();
  this->cs_isr_ = this->cs_pin_->to_isr();

  // Initialize state (critical - ISRs will read these)
  this->bit_count_ = 0;
  this->cs_active_ = false;
  this->data_miso_ = 0;
  this->data_mosi_ = 0;
  this->buffer_count_ = 0;
  this->write_idx_ = 0;
  this->read_idx_ = 0;

  // Register ISR handlers using the framework GPIO wrapper
  // This keeps all interrupt setup within ESPHome's GPIO abstraction
  this->clk_pin_->attach_interrupt(clk_isr_handler, this, gpio::INTERRUPT_RISING_EDGE);
  this->cs_pin_->attach_interrupt(cs_isr_handler, this, gpio::INTERRUPT_ANY_EDGE);

  // FINALLY: Mark component as initialized
  // This allows ISRs to start firing
  this->initialized_ = true;

  ESP_LOGI(TAG, "Whirlpool SPI monitor setup complete");
}

void WhirlpoolSPI::dump_config() {
  ESP_LOGCONFIG(TAG, "Whirlpool SPI Monitor:");
  LOG_PIN("  MISO Pin: ", this->miso_pin_);
  LOG_PIN("  MOSI Pin: ", this->mosi_pin_);
  LOG_PIN("  CS Pin: ", this->cs_pin_);
  LOG_PIN("  CLK Pin: ", this->clk_pin_);
  ESP_LOGCONFIG(TAG, "  Expected Bits: %u  Buffer Size: %u/%u  Error Frames: %u", this->expected_bits_,
                this->buffer_size_, MAX_RECORDS, this->error_frames_);
}

void WhirlpoolSPI::loop() {
  // Snapshot buffer status before processing
  uint32_t initial_count;
  {
    InterruptLock lock;
    initial_count = this->buffer_count_;
  }

  // Log buffer status if queue is building up (only log periodically to avoid spam)
  static uint32_t last_status_log = 0;
  uint32_t now_ms = millis();
  if (initial_count > this->buffer_size_ / 2 && (now_ms - last_status_log) > 1000) {
    // Buffer is >50% full - might be a throughput issue
    ESP_LOGW(TAG, "Buffer accumulation: %d/%d transfers queued (loop may be slow)", initial_count, this->buffer_size_);
    last_status_log = now_ms;
  }

  uint32_t processed = 0;
  // Process all buffered transfers from the ring buffer.
  // This is safe to drain fully because the heavy publish_state() calls
  // have been moved to update(); each iteration here is just an
  // InterruptLock + lightweight state-machine work.
  while (this->buffer_count_ > 0) {
    uint32_t bit_count;
    uint64_t miso_data;
    uint64_t mosi_data;

    // Block interrupts to safely read and remove from ring buffer
    {
      InterruptLock lock;
      if (this->buffer_count_ == 0) {
        break;  // Safety check in case buffer emptied
      }

      bit_count = this->transfer_buffer_[this->read_idx_].bit_count;
      miso_data = this->transfer_buffer_[this->read_idx_].data_miso;
      mosi_data = this->transfer_buffer_[this->read_idx_].data_mosi;

      this->read_idx_ = (this->read_idx_ + 1) % this->buffer_size_;
      this->buffer_count_--;
    }
    processed++;

    // Check if this frame has the expected number of bits
    if (bit_count != this->expected_bits_) {
      if (bit_count > 0) {
        // Track rejected transfers
        this->error_frames_++;
        if (bit_count < this->expected_bits_) {
          this->error_under_++;
        } else {
          this->error_over_++;
        }
        ESP_LOGV(TAG, "REJECTED Bits: %2d / %2d  |  MISO: %016llx  |  MOSI: %016llx", bit_count, this->expected_bits_,
                 (unsigned long long) miso_data, (unsigned long long) mosi_data);
      }

      // Bad frame: if we were mid-group, drop the entire group
      if (this->frame_group_synced_ && this->frame_group_pos_ > 0) {
        ESP_LOGV(TAG, "Frame group dropped at position %u (bad bit count %u)", this->frame_group_pos_, bit_count);
        this->groups_dropped_++;
        this->frame_group_pos_ = 0;
        this->frame_group_synced_ = false;
        // Invalidate debounce history — stream was disrupted
        this->history_count_ = 0;
      }
      continue;
    }

    // Valid frame - log it
    this->accum_valid_frames_++;
    ESP_LOGVV(TAG, "Bits: %2d  |  MISO: %016llx  |  MOSI: %016llx", bit_count, (unsigned long long) miso_data,
              (unsigned long long) mosi_data);

    // --- Frame-group state machine ---
    // Extract the upper 32 bits of MISO to check for the marker
    uint32_t miso_upper = static_cast<uint32_t>(miso_data >> 32);
    bool is_marker = (miso_upper == MARKER_MISO_UPPER);

    if (is_marker) {
      // Marker frame detected - (re)start a new group
      if (this->frame_group_synced_ && this->frame_group_pos_ > 0) {
        // We were mid-group but got a new marker - drop the incomplete group
        ESP_LOGV(TAG, "Frame group dropped at position %u (new marker arrived early)", this->frame_group_pos_);
        this->groups_dropped_++;
        // Invalidate debounce history — stream was disrupted
        this->history_count_ = 0;
      }
      this->frame_group_synced_ = true;
      this->frame_group_pos_ = 0;
      this->frame_group_[0].bit_count = bit_count;
      this->frame_group_[0].data_miso = miso_data;
      this->frame_group_[0].data_mosi = mosi_data;
      this->frame_group_pos_ = 1;  // Next expected position
    } else if (this->frame_group_synced_ && this->frame_group_pos_ > 0) {
      // Non-marker frame while we're accumulating a group
      uint32_t pos = this->frame_group_pos_;
      this->frame_group_[pos].bit_count = bit_count;
      this->frame_group_[pos].data_miso = miso_data;
      this->frame_group_[pos].data_mosi = mosi_data;
      this->frame_group_pos_ = pos + 1;

      // Check if the group is now complete
      if (this->frame_group_pos_ == FRAME_GROUP_SIZE) {
        this->frame_group_pos_ = 0;
        // Group stays synced - we'll look for the next marker

        // Run plausibility checks before accepting
        if (!this->validate_frame_group_()) {
          this->groups_failed_plausibility_++;
          ESP_LOGV(TAG, "Frame group failed plausibility check (total failures: %u)",
                   this->groups_failed_plausibility_);
          continue;
        }

        this->groups_completed_++;

        // --- Consecutive-match debounce ---
        // Write the validated group into the next history slot.
        memcpy(this->frame_group_history_[this->history_write_idx_], this->frame_group_,
               sizeof(Transfer) * FRAME_GROUP_SIZE);
        this->history_write_idx_ = (this->history_write_idx_ + 1) % DEBOUNCE_DEPTH;
        if (this->history_count_ < DEBOUNCE_DEPTH) {
          this->history_count_++;
        }

        // Only publish once all slots are filled and every slot matches slot 0.
        bool consensus = (this->history_count_ >= DEBOUNCE_DEPTH);
        if (consensus) {
          for (uint32_t d = 1; d < DEBOUNCE_DEPTH; d++) {
            if (!frame_groups_match_(this->frame_group_history_[0], this->frame_group_history_[d])) {
              consensus = false;
              break;
            }
          }
        }

        if (consensus) {
          ESP_LOGV(TAG, "Frame group #%u published (debounce %u/%u confirmed):", this->groups_completed_,
                   DEBOUNCE_DEPTH, DEBOUNCE_DEPTH);
          for (uint32_t i = 0; i < FRAME_GROUP_SIZE; i++) {
            ESP_LOGV(TAG, "  [%u] MISO: %016llx  MOSI: %016llx", i,
                     (unsigned long long) this->frame_group_[i].data_miso,
                     (unsigned long long) this->frame_group_[i].data_mosi);
          }
          this->publish_frame_group_();
        } else {
          ESP_LOGV(TAG, "Frame group #%u awaiting debounce consensus (%u/%u)", this->groups_completed_,
                   this->history_count_, DEBOUNCE_DEPTH);
        }
      }
    }
    // else: non-marker frame and not synced - just ignore it
  }

  // Accumulate counters for the next update() cycle
  this->accum_processed_ += processed;
  this->last_buffer_queue_ = initial_count;
}

void WhirlpoolSPI::update() {
  float interval_sec = this->get_update_interval() / 1000.0f;

  // Convert accumulated counts to per-second rates
  float transfers_ps = (interval_sec > 0) ? this->accum_processed_ / interval_sec : 0;
  float valid_frames_ps = (interval_sec > 0) ? this->accum_valid_frames_ / interval_sec : 0;
  float error_frames_ps = (interval_sec > 0) ? this->error_frames_ / interval_sec : 0;
  float error_under_ps = (interval_sec > 0) ? this->error_under_ / interval_sec : 0;
  float error_over_ps = (interval_sec > 0) ? this->error_over_ / interval_sec : 0;
  float groups_completed_ps = (interval_sec > 0) ? this->groups_completed_ / interval_sec : 0;
  float groups_dropped_ps = (interval_sec > 0) ? this->groups_dropped_ / interval_sec : 0;
  float groups_failed_ps = (interval_sec > 0) ? this->groups_failed_plausibility_ / interval_sec : 0;

  ESP_LOGD(TAG, "Rates (/s): transfers=%.1f, valid=%.1f, errors=%.1f (under=%.1f, over=%.1f), queue=%d/%d",
           transfers_ps, valid_frames_ps, error_frames_ps, error_under_ps, error_over_ps, this->last_buffer_queue_,
           this->buffer_size_);
  ESP_LOGD(TAG, "Groups (/s): completed=%.2f, dropped=%.2f, failed_plausibility=%.2f", groups_completed_ps,
           groups_dropped_ps, groups_failed_ps);

  // Publish to diagnostic sensors (all rates are per-second; buffer_queue is a snapshot)
  if (this->transfers_per_sec_sensor_ != nullptr)
    this->transfers_per_sec_sensor_->publish_state(transfers_ps);
  if (this->buffer_queue_sensor_ != nullptr)
    this->buffer_queue_sensor_->publish_state(this->last_buffer_queue_);
  if (this->valid_frames_sensor_ != nullptr)
    this->valid_frames_sensor_->publish_state(valid_frames_ps);
  if (this->error_frames_sensor_ != nullptr)
    this->error_frames_sensor_->publish_state(error_frames_ps);
  if (this->error_under_sensor_ != nullptr)
    this->error_under_sensor_->publish_state(error_under_ps);
  if (this->error_over_sensor_ != nullptr)
    this->error_over_sensor_->publish_state(error_over_ps);
  if (this->groups_completed_sensor_ != nullptr)
    this->groups_completed_sensor_->publish_state(groups_completed_ps);
  if (this->groups_dropped_sensor_ != nullptr)
    this->groups_dropped_sensor_->publish_state(groups_dropped_ps);
  if (this->groups_failed_plausibility_sensor_ != nullptr)
    this->groups_failed_plausibility_sensor_->publish_state(groups_failed_ps);

  // Reset accumulators for next interval
  this->accum_processed_ = 0;
  this->accum_valid_frames_ = 0;
  this->error_frames_ = 0;
  this->error_under_ = 0;
  this->error_over_ = 0;
  this->groups_completed_ = 0;
  this->groups_dropped_ = 0;
  this->groups_failed_plausibility_ = 0;
}

bool WhirlpoolSPI::validate_frame_group_() {
  // --- All MISO frames: upper 32 bits must be a single byte repeated 4 times ---
  // e.g. 0x00000000, 0x02020202, 0x12121212, etc.
  for (uint32_t i = 0; i < FRAME_GROUP_SIZE; i++) {
    uint32_t upper = static_cast<uint32_t>(this->frame_group_[i].data_miso >> 32);
    uint8_t byte_val = upper & 0xFF;
    uint32_t expected = byte_val * 0x01010101u;
    if (upper != expected) {
      ESP_LOGV(TAG, "Plausibility FAIL: frame %u MISO upper=0x%08X (expected repeated 0x%02X -> 0x%08X)", i, upper,
               byte_val, expected);
      return false;
    }
  }

  return true;
}

bool WhirlpoolSPI::frame_groups_match_(const Transfer *a, const Transfer *b) {
  for (uint32_t i = 0; i < FRAME_GROUP_SIZE; i++) {
    if (a[i].data_miso != b[i].data_miso || a[i].data_mosi != b[i].data_mosi)
      return false;
  }
  return true;
}

void WhirlpoolSPI::publish_frame_group_() {
  // Format upper 32 bits as 8-char hex string (lower 32 bits are always zero)
  char buf[9];  // 8 hex chars + null terminator

  // Helper lambda to format and publish
  auto publish = [&buf](text_sensor::TextSensor *sensor, uint64_t data) {
    if (sensor == nullptr)
      return;
    snprintf(buf, sizeof(buf), "%08X", static_cast<uint32_t>(data >> 32));
    sensor->publish_state(buf);
  };

  publish(this->frame0_miso_text_sensor_, this->frame_group_[0].data_miso);
  publish(this->frame0_mosi_text_sensor_, this->frame_group_[0].data_mosi);
  publish(this->frame1_miso_text_sensor_, this->frame_group_[1].data_miso);
  publish(this->frame1_mosi_text_sensor_, this->frame_group_[1].data_mosi);
  publish(this->frame2_miso_text_sensor_, this->frame_group_[2].data_miso);
  publish(this->frame2_mosi_text_sensor_, this->frame_group_[2].data_mosi);
  publish(this->frame3_miso_text_sensor_, this->frame_group_[3].data_miso);
  publish(this->frame3_mosi_text_sensor_, this->frame_group_[3].data_mosi);
  publish(this->frame4_miso_text_sensor_, this->frame_group_[4].data_miso);
  publish(this->frame4_mosi_text_sensor_, this->frame_group_[4].data_mosi);

  // --- Derived values from validated frame data ---
  // Raw byte from frame 1 MISO has bit 0 always 0; bits [4:1] form the index.
  uint8_t selector_raw = static_cast<uint8_t>(this->frame_group_[1].data_miso >> 32);
  uint8_t selector_idx = selector_raw >> 1;  // 0–15

  // Selector position (shifted index)
  if (this->selector_position_sensor_ != nullptr) {
    this->selector_position_sensor_->publish_state(selector_idx);
  }

  // Selector position name (mapped from YAML-configured labels)
  if (this->selector_position_name_text_sensor_ != nullptr) {
    if (selector_idx < SELECTOR_LABEL_COUNT && !this->selector_labels_[selector_idx].empty()) {
      this->selector_position_name_text_sensor_->publish_state(this->selector_labels_[selector_idx]);
    } else {
      // No label configured for this position — publish index
      snprintf(buf, sizeof(buf), "%u", selector_idx);
      this->selector_position_name_text_sensor_->publish_state(buf);
    }
  }

  // --- Butt5on binary sensors (derived from frame MISO upper bytes) ---
  uint8_t f2miso_byte = static_cast<uint8_t>(this->frame_group_[2].data_miso >> 32);
  uint8_t f3miso_byte = static_cast<uint8_t>(this->frame_group_[3].data_miso >> 32);
  uint8_t f4miso_byte = static_cast<uint8_t>(this->frame_group_[4].data_miso >> 32);
  uint8_t f5miso_byte = static_cast<uint8_t>(this->frame_group_[5].data_miso >> 32);

  if (this->power_button_binary_sensor_ != nullptr)
    this->power_button_binary_sensor_->publish_state((f3miso_byte & BUTTON_MASK_POWER) != 0);
  if (this->delay_wash_binary_sensor_ != nullptr)
    this->delay_wash_binary_sensor_->publish_state((f3miso_byte & BUTTON_MASK_DELAY_WASH) != 0);
  if (this->start_pause_binary_sensor_ != nullptr)
    this->start_pause_binary_sensor_->publish_state((f3miso_byte & BUTTON_MASK_START_PAUSE) != 0);
  if (this->cycle_signal_binary_sensor_ != nullptr)
    this->cycle_signal_binary_sensor_->publish_state((f3miso_byte & BUTTON_MASK_CYCLE_SIGNAL) != 0);
  if (this->ecoboost_binary_sensor_ != nullptr)
    this->ecoboost_binary_sensor_->publish_state((f4miso_byte & BUTTON_MASK_ECOBOOST) != 0);
  if (this->extra_rinse_binary_sensor_ != nullptr)
    this->extra_rinse_binary_sensor_->publish_state((f4miso_byte & BUTTON_MASK_EXTRA_RINSE) != 0);
  if (this->deep_clean_binary_sensor_ != nullptr)
    this->deep_clean_binary_sensor_->publish_state((f4miso_byte & BUTTON_MASK_DEEP_CLEAN) != 0);
  if (this->soil_level_binary_sensor_ != nullptr)
    this->soil_level_binary_sensor_->publish_state((f4miso_byte & BUTTON_MASK_SOIL_LEVEL) != 0);
  if (this->spin_speed_binary_sensor_ != nullptr)
    this->spin_speed_binary_sensor_->publish_state((f4miso_byte & BUTTON_MASK_SPIN_SPEED) != 0);
  if (this->wash_temp_binary_sensor_ != nullptr)
    this->wash_temp_binary_sensor_->publish_state((f4miso_byte & BUTTON_MASK_WASH_TEMP) != 0);
  if (this->dispensers_binary_sensor_ != nullptr)
    this->dispensers_binary_sensor_->publish_state((f4miso_byte & BUTTON_MASK_DISPENSERS) != 0);
}

}  // namespace whirlpool_spi
}  // namespace esphome
