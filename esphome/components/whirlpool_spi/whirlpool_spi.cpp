#include "whirlpool_spi.h"
#include "esphome/core/application.h"
#include "esphome/core/hal.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"
#include <cinttypes>
#include <cstdio>

#include "soc/gpio_reg.h"
#include "soc/soc.h"

#ifdef USE_WHIRLPOOL_SPI_PERIPHERAL
#include "esp_cpu.h"
#include "esp_intr_alloc.h"
#include "esp_private/spi_slave_internal.h"  // NOLINT
#include "esp_rom_gpio.h"
#include "soc/spi_periph.h"
#endif

namespace esphome::whirlpool_spi {

ESPHOME_LOG_TAG(TAG, "whirlpool_spi");

// Always inlined so it lands in the calling ISR's IRAM section.
ESPHOME_ALWAYS_INLINE inline void WhirlpoolSPI::push_transfer_(uint32_t bit_count, uint32_t miso, uint32_t mosi) {
  const uint32_t head = this->ring_head_.load(std::memory_order_relaxed);
  const uint32_t used = head - this->ring_tail_.load(std::memory_order_acquire);
  if (used >= MAX_RECORDS) {
    this->ring_overflows_.store(this->ring_overflows_.load(std::memory_order_relaxed) + 1, std::memory_order_relaxed);
    return;
  }
  this->transfer_buffer_[head & RING_MASK] = {bit_count, miso, mosi};
  this->ring_head_.store(head + 1, std::memory_order_release);
  if (used + 1 > this->ring_peak_.load(std::memory_order_relaxed))
    this->ring_peak_.store(used + 1, std::memory_order_relaxed);
}

#ifdef USE_WHIRLPOOL_SPI_PERIPHERAL
// Measured on the washer's bus: mode 1 is the only mode that reads MOSI correctly. Mode 0 reads it one bit late, and
// modes 2 and 3 sample it as it changes, which merges adjacent bits.
static constexpr int CAPTURE_SPI_MODE = 1;

// The two channels complete the same frame within microseconds of each other, and the next frame is about a
// millisecond away, so a partner older than this can only be left over from a frame the other channel missed.
static constexpr uint32_t CHANNEL_PAIR_WINDOW_CYCLES = 300 * CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ;

// Both channels' interrupts have the same priority on the same core, so this never preempts itself.
void IRAM_ATTR WhirlpoolSPI::channel_transfer_done_(spi_slave_transaction_t *trans) {  // NOLINT
  // Read the pins before anything else, while the data line still holds the frame's last bit
  const uint32_t in = REG_READ(GPIO_IN_REG);
  auto *channel = static_cast<CaptureChannel *>(trans->user);
  WhirlpoolSPI *c = channel->parent;
  channel->end_inputs = in;

  // Received bytes land in memory in wire order, so swap to put the first bit on the wire at bit 31.
  channel->data = __builtin_bswap32(channel->rx_buffer[0]);
  channel->bit_count = trans->trans_len;
  channel->completed_at = esp_cpu_get_cycle_count();
  channel->pending = true;

  CaptureChannel *other = channel == &c->miso_channel_ ? &c->mosi_channel_ : &c->miso_channel_;
  if (other->pending) {
    if (channel->completed_at - other->completed_at <= CHANNEL_PAIR_WINDOW_CYCLES) {
      // If the channels disagree on length, report whichever is wrong so the frame is rejected.
      const uint32_t bit_count = channel->bit_count == EXPECTED_BITS ? other->bit_count : channel->bit_count;
      // A CS pulse with no clocks carries no data, so don't let it disturb the frame-group state machine.
      if (bit_count != 0) {
        // On this bus MISO only changes after each rising clock edge, so the peripheral samples every MISO bit one
        // clock late and the frame's last bit arrives after the last edge. Shift the sampled bits back into place
        // and take the last bit from the line, which holds it until the next frame's first clock edge.
        const uint32_t inputs = c->miso_channel_.end_inputs;
        const uint32_t miso = (c->miso_channel_.data << 1) | ((inputs >> c->miso_channel_.data_bit) & 1);
        // CS low again means this ran so late that the next frame had started and the line may have moved on.
        // Count the frame as one bit short so it's rejected.
        const bool last_bit_read = (inputs >> c->cs_bit_) & 1;
        const uint32_t reported_bits = bit_count == EXPECTED_BITS && !last_bit_read ? EXPECTED_BITS - 1 : bit_count;
        c->push_transfer_(reported_bits, miso, c->mosi_channel_.data);
      }
      channel->pending = false;
    } else {
      c->channel_desyncs_.store(c->channel_desyncs_.load(std::memory_order_relaxed) + 1, std::memory_order_relaxed);
    }
    other->pending = false;
  }

  // Re-arm from here rather than from loop(), so capture never depends on the main loop keeping up. This transaction
  // is the only one the channel ever uses and the driver has already dequeued it, so its queue always has room.
  spi_slave_queue_trans_isr(channel->host, trans);  // NOLINT
}
#else
// Pulses on CLK shorter than this are ignored. Real CLK phases are at least 4.5 us and the glitches on the washer's CLK
// line are under 141 ns. The counter registers an edge this long after it happens, which is still well before the
// CLK interrupt gets to read it.
static constexpr uint32_t CLK_GLITCH_FILTER_NS = 500;

// ISR for CS transitions (both edges).
void IRAM_ATTR WhirlpoolSPI::cs_isr_(WhirlpoolSPI *c) {
  const uint32_t in = REG_READ(GPIO_IN_REG);

  if (((in >> c->cs_bit_) & 1) == 0) {
    // CS low: start of transfer. Clearing here also discards any clock edges seen while CS was high.
    c->bit_count_ = 0;
    c->miso_shift_ = 0;
    c->mosi_shift_ = 0;
    pcnt_unit_get_count(c->clk_counter_, &c->last_clk_count_);
    return;
  }

  // CS high: end of transfer.
  // A CS pulse with no clocks carries no data, so don't let it disturb the frame-group state machine.
  if (c->bit_count_ != 0) {
    // Each falling edge sampled the MISO bit before its own, so shift those into place and take the last bit from
    // the line, which holds it until the next frame's first clock edge.
    c->push_transfer_(c->bit_count_, (c->miso_shift_ << 1) | ((in >> c->miso_bit_) & 1), c->mosi_shift_);
  }
}

// ISR for CLK falling edges. On this bus MOSI changes at the falling edge and MISO a few microseconds after the rising
// edge, so just after a falling edge both are settled: MOSI holds the bit about to be clocked and MISO the one before.
// Every glitch on CLK raises an interrupt of its own, so an edge only counts if the filtered pulse counter saw it too.
void IRAM_ATTR WhirlpoolSPI::clk_isr_(WhirlpoolSPI *c) {
  const uint32_t in = REG_READ(GPIO_IN_REG);
  int count;
  pcnt_unit_get_count(c->clk_counter_, &count);
  if (count == c->last_clk_count_) {
    c->clk_glitches_.store(c->clk_glitches_.load(std::memory_order_relaxed) + 1, std::memory_order_relaxed);
    return;
  }
  c->last_clk_count_ = count;
  c->miso_shift_ = (c->miso_shift_ << 1) | ((in >> c->miso_bit_) & 1);
  c->mosi_shift_ = (c->mosi_shift_ << 1) | ((in >> c->mosi_bit_) & 1);
  c->bit_count_++;
}
#endif  // USE_WHIRLPOOL_SPI_PERIPHERAL

void WhirlpoolSPI::setup() {
  // Inputs only: this monitors a bus that belongs to the washer
  this->miso_pin_->setup();
  this->mosi_pin_->setup();
  this->cs_pin_->setup();
  this->clk_pin_->setup();
  this->cs_bit_ = this->cs_pin_->get_pin();

#ifdef USE_WHIRLPOOL_SPI_PERIPHERAL
  // Each peripheral has one data input, so capturing both lines takes two, with the washer's MISO fed into the second
  // peripheral's MOSI input.
  if (!this->start_capture_channel_(this->miso_channel_, SPI3_HOST, this->miso_pin_) ||
      !this->start_capture_channel_(this->mosi_channel_, SPI2_HOST, this->mosi_pin_)) {
    this->mark_failed();
    return;
  }
#else
  this->miso_bit_ = this->miso_pin_->get_pin();
  this->mosi_bit_ = this->mosi_pin_->get_pin();
  if (!this->start_clk_counter_()) {
    this->mark_failed();
    return;
  }
#endif

  // This is a passive tap on the washer's bus: if any of its pins can drive, stop and say so.
  const uint32_t bus_pins = (1UL << this->miso_pin_->get_pin()) | (1UL << this->mosi_pin_->get_pin()) |
                            (1UL << this->cs_pin_->get_pin()) | (1UL << this->clk_pin_->get_pin());
  if ((REG_READ(GPIO_ENABLE_REG) & bus_pins) != 0) {
    ESP_LOGE(TAG, "A bus pin is configured as an output (GPIO_ENABLE=0x%08" PRIX32 ")", REG_READ(GPIO_ENABLE_REG));
    this->mark_failed();
    return;
  }

#ifndef USE_WHIRLPOOL_SPI_PERIPHERAL
  this->clk_pin_->attach_interrupt(WhirlpoolSPI::clk_isr_, this, gpio::INTERRUPT_FALLING_EDGE);
  this->cs_pin_->attach_interrupt(WhirlpoolSPI::cs_isr_, this, gpio::INTERRUPT_ANY_EDGE);
#endif
}

#ifdef USE_WHIRLPOOL_SPI_PERIPHERAL
bool WhirlpoolSPI::start_capture_channel_(CaptureChannel &channel, spi_host_device_t host, InternalGPIOPin *data_pin) {
  // Every pin is -1 so the driver leaves the pads alone. Given real pins it sets them to input-output and connects the
  // peripheral's outputs, before the peripheral is even set up as a receiver, which has no place on a passive tap.
  spi_bus_config_t bus{};
  bus.mosi_io_num = -1;
  bus.miso_io_num = -1;
  bus.sclk_io_num = -1;
  bus.quadwp_io_num = -1;
  bus.quadhd_io_num = -1;
  bus.intr_flags = ESP_INTR_FLAG_LEVEL3;

  spi_slave_interface_config_t device{};  // NOLINT
  device.spics_io_num = -1;
  device.flags = SPI_SLAVE_NO_RETURN_RESULT;  // NOLINT
  device.queue_size = 1;
  device.mode = CAPTURE_SPI_MODE;
  device.post_trans_cb = WhirlpoolSPI::channel_transfer_done_;

  esp_err_t err = spi_slave_initialize(host, &bus, &device, SPI_DMA_DISABLED);  // NOLINT
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "SPI driver init failed on host %d: %s", host, esp_err_to_name(err));
    return false;
  }

  // Input-only routing. The pads were set up as plain inputs by setup(), and this adds no path back out to them.
  const spi_signal_conn_t &signals = spi_periph_signal[host];
  esp_rom_gpio_connect_in_signal(this->clk_pin_->get_pin(), signals.spiclk_in, false);
  esp_rom_gpio_connect_in_signal(this->cs_pin_->get_pin(), signals.spics_in, false);
  esp_rom_gpio_connect_in_signal(data_pin->get_pin(), signals.spid_in, false);

  channel.parent = this;
  channel.host = host;
  channel.data_bit = data_pin->get_pin();
  channel.transaction.length = sizeof(channel.rx_buffer) * 8;
  channel.transaction.rx_buffer = channel.rx_buffer;
  channel.transaction.user = &channel;
  err = spi_slave_queue_trans(host, &channel.transaction, 0);  // NOLINT
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "SPI driver queue failed on host %d: %s", host, esp_err_to_name(err));
    return false;
  }
  return true;
}
#endif

#ifndef USE_WHIRLPOOL_SPI_PERIPHERAL
bool WhirlpoolSPI::start_clk_counter_() {
  pcnt_unit_config_t unit_config{};
  unit_config.low_limit = -1;
  unit_config.high_limit = INT16_MAX;  // The count wraps to 0 here, which is fine: only changes in it matter
  pcnt_glitch_filter_config_t filter{};
  filter.max_glitch_ns = CLK_GLITCH_FILTER_NS;
  pcnt_chan_config_t channel_config{};
  channel_config.edge_gpio_num = this->clk_pin_->get_pin();
  channel_config.level_gpio_num = -1;
  channel_config.flags.virt_level_io_level = 1;
  pcnt_channel_handle_t channel = nullptr;
  if (pcnt_new_unit(&unit_config, &this->clk_counter_) != ESP_OK ||
      pcnt_unit_set_glitch_filter(this->clk_counter_, &filter) != ESP_OK ||
      pcnt_new_channel(this->clk_counter_, &channel_config, &channel) != ESP_OK ||
      pcnt_channel_set_edge_action(channel, PCNT_CHANNEL_EDGE_ACTION_HOLD, PCNT_CHANNEL_EDGE_ACTION_INCREASE) !=
          ESP_OK ||
      pcnt_channel_set_level_action(channel, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_KEEP) !=
          ESP_OK ||
      pcnt_unit_enable(this->clk_counter_) != ESP_OK || pcnt_unit_start(this->clk_counter_) != ESP_OK) {
    ESP_LOGE(TAG, "CLK pulse counter setup failed");
    return false;
  }
  // The driver turns on CLK's pull-up. Put back the pin mode from the configuration; the counter's input stays routed.
  this->clk_pin_->setup();
  return true;
}
#endif

void WhirlpoolSPI::dump_config() {
  ESP_LOGCONFIG(TAG, "Whirlpool SPI Monitor:");
  LOG_UPDATE_INTERVAL(this);
  LOG_PIN("  MISO Pin: ", this->miso_pin_);
  LOG_PIN("  MOSI Pin: ", this->mosi_pin_);
  LOG_PIN("  CS Pin: ", this->cs_pin_);
  LOG_PIN("  CLK Pin: ", this->clk_pin_);
#ifdef USE_WHIRLPOOL_SPI_PERIPHERAL
  ESP_LOGCONFIG(TAG, "  Capture: SPI peripherals");
#else
  ESP_LOGCONFIG(TAG, "  Capture: GPIO interrupts, CLK glitch filter %" PRIu32 " ns", CLK_GLITCH_FILTER_NS);
#endif
  ESP_LOGCONFIG(TAG, "  Expected Bits: %" PRIu32 "  Buffer Size: %" PRIu32, EXPECTED_BITS, MAX_RECORDS);
}

void WhirlpoolSPI::loop() {
  // Work through what was queued on entry. Anything arriving meanwhile waits for the next pass,
  // so a busy bus can't hold up the main loop.
  const uint32_t head = this->ring_head_.load(std::memory_order_acquire);
  uint32_t tail = this->ring_tail_.load(std::memory_order_relaxed);
  const uint32_t queued = head - tail;

  // Warn if the queue is building up, at most once a second
  const uint32_t now = App.get_loop_component_start_time();
  if (queued > MAX_RECORDS / 2 && now - this->last_status_log_ > 1000) {
    ESP_LOGW(TAG, "Buffer accumulation: %" PRIu32 "/%" PRIu32 " transfers queued (loop may be slow)", queued,
             MAX_RECORDS);
    this->last_status_log_ = now;
  }

  for (; tail != head; tail++) {
    // Copy the slot out before releasing it to the ISR
    const Transfer frame = this->transfer_buffer_[tail & RING_MASK];
    this->ring_tail_.store(tail + 1, std::memory_order_release);
    this->process_frame_(frame);
  }

  this->accum_processed_ += queued;
  this->last_buffer_queue_ = queued;
}

void WhirlpoolSPI::process_frame_(const Transfer &frame) {
  // Check if this frame has the expected number of bits
  if (frame.bit_count != EXPECTED_BITS) {
    this->error_frames_++;
    if (frame.bit_count < EXPECTED_BITS) {
      this->error_under_++;
    } else {
      this->error_over_++;
    }
    ESP_LOGV(TAG, "REJECTED Bits: %2" PRIu32 " / %2" PRIu32 "  |  MISO: %08" PRIX32 "  |  MOSI: %08" PRIX32,
             frame.bit_count, EXPECTED_BITS, frame.data_miso, frame.data_mosi);

    // Bad frame: if we were mid-group, drop the entire group
    if (this->frame_group_pos_ > 0) {
      ESP_LOGV(TAG, "Frame group dropped at position %" PRIu32 " (bad bit count %" PRIu32 ")", this->frame_group_pos_,
               frame.bit_count);
      this->groups_dropped_++;
      this->frame_group_pos_ = 0;
      // Invalidate debounce history: the stream was disrupted
      this->match_count_ = 0;
    }
    return;
  }

  this->accum_valid_frames_++;
  ESP_LOGVV(TAG, "Bits: %2" PRIu32 "  |  MISO: %08" PRIX32 "  |  MOSI: %08" PRIX32, frame.bit_count, frame.data_miso,
            frame.data_mosi);

  // --- Frame-group state machine ---
  if (frame.data_miso == MARKER_MISO) {
    // Marker frame detected - (re)start a new group
    if (this->frame_group_pos_ > 0) {
      // We were mid-group but got a new marker - drop the incomplete group
      ESP_LOGV(TAG, "Frame group dropped at position %" PRIu32 " (new marker arrived early)", this->frame_group_pos_);
      this->groups_dropped_++;
      // Invalidate debounce history: the stream was disrupted
      this->match_count_ = 0;
    }
    this->frame_group_pos_ = 0;
  } else if (this->frame_group_pos_ == 0) {
    return;  // Non-marker frame while waiting for a marker
  }

  const uint32_t pos = this->frame_group_pos_;
  this->frame_group_.miso[pos] = frame.data_miso;
  this->frame_group_.mosi[pos] = frame.data_mosi;

  if (pos + 1 < FRAME_GROUP_SIZE) {
    this->frame_group_pos_ = pos + 1;
    return;
  }
  this->frame_group_pos_ = 0;
  this->process_complete_group_();
}

void WhirlpoolSPI::process_complete_group_() {
  // Run plausibility checks before accepting
  if (!this->validate_frame_group_()) {
    this->groups_failed_plausibility_++;
    this->match_count_ = 0;
    ESP_LOGV(TAG, "Frame group failed plausibility check");
    return;
  }

  this->groups_completed_++;

  // --- Consecutive-match debounce ---
  if (this->match_count_ > 0 && this->frame_group_ == this->candidate_group_) {
    if (this->match_count_ < DEBOUNCE_DEPTH)
      this->match_count_++;
  } else {
    this->candidate_group_ = this->frame_group_;
    this->match_count_ = 1;
  }

  if (this->match_count_ < DEBOUNCE_DEPTH) {
    ESP_LOGV(TAG, "Frame group awaiting debounce consensus (%u/%" PRIu32 ")", this->match_count_, DEBOUNCE_DEPTH);
    return;
  }

  // The bus repeats the same group hundreds of times a second; only a change is worth publishing.
  if (this->has_published_ && this->frame_group_ == this->published_group_)
    return;

  this->published_group_ = this->frame_group_;
  this->has_published_ = true;
  for (uint32_t i = 0; i < FRAME_GROUP_SIZE; i++) {
    ESP_LOGV(TAG, "  [%" PRIu32 "] MISO: %08" PRIX32 "  MOSI: %08" PRIX32, i, this->frame_group_.miso[i],
             this->frame_group_.mosi[i]);
  }
  this->publish_frame_group_();
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
  const uint32_t overflows = this->ring_overflows_.load(std::memory_order_relaxed);
  const uint32_t peak = this->ring_peak_.exchange(0, std::memory_order_relaxed);

  ESP_LOGD(TAG,
           "Rates (/s): transfers=%.1f, valid=%.1f, errors=%.1f (under=%.1f, over=%.1f), queue=%" PRIu32 "/%" PRIu32
           ", peak=%" PRIu32 ", overflows=%" PRIu32,
           transfers_ps, valid_frames_ps, error_frames_ps, error_under_ps, error_over_ps, this->last_buffer_queue_,
           MAX_RECORDS, peak, overflows);
  ESP_LOGD(TAG, "Groups (/s): completed=%.2f, dropped=%.2f, failed_plausibility=%.2f", groups_completed_ps,
           groups_dropped_ps, groups_failed_ps);
#ifdef USE_WHIRLPOOL_SPI_PERIPHERAL
  ESP_LOGD(TAG, "Channel desyncs since boot: %" PRIu32, this->channel_desyncs_.load(std::memory_order_relaxed));
#else
  ESP_LOGD(TAG, "CLK glitches rejected since boot: %" PRIu32, this->clk_glitches_.load(std::memory_order_relaxed));
#endif

  // Publish to diagnostic sensors (all rates are per-second; buffer_queue is a snapshot)
  if (this->transfers_per_sec_sensor_ != nullptr)
    this->transfers_per_sec_sensor_->publish_state(transfers_ps);
  if (this->buffer_queue_sensor_ != nullptr)
    this->buffer_queue_sensor_->publish_state(this->last_buffer_queue_);
  if (this->buffer_overflows_sensor_ != nullptr)
    this->buffer_overflows_sensor_->publish_state(overflows);
  if (this->buffer_peak_sensor_ != nullptr)
    this->buffer_peak_sensor_->publish_state(peak);
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
  // --- All MISO frames: must be a single byte repeated 4 times ---
  // e.g. 0x00000000, 0x02020202, 0x12121212, etc.
  for (uint32_t i = 0; i < FRAME_GROUP_SIZE; i++) {
    const uint32_t miso = this->frame_group_.miso[i];
    const uint8_t byte_val = miso & 0xFF;
    const uint32_t expected = byte_val * 0x01010101u;
    if (miso != expected) {
      ESP_LOGV(TAG,
               "Plausibility FAIL: frame %" PRIu32 " MISO=0x%08" PRIX32 " (expected repeated 0x%02X -> 0x%08" PRIX32
               ")",
               i, miso, byte_val, expected);
      return false;
    }
  }

  return true;
}

void WhirlpoolSPI::publish_frame_group_() {
  char buf[9];  // 8 hex chars + null terminator

  // Helper lambda to format and publish
  auto publish = [&buf](text_sensor::TextSensor *sensor, uint32_t data) {
    if (sensor == nullptr)
      return;
    snprintf(buf, sizeof(buf), "%08" PRIX32, data);
    sensor->publish_state(buf);
  };

  publish(this->frame0_miso_text_sensor_, this->frame_group_.miso[0]);
  publish(this->frame0_mosi_text_sensor_, this->frame_group_.mosi[0]);
  publish(this->frame1_miso_text_sensor_, this->frame_group_.miso[1]);
  publish(this->frame1_mosi_text_sensor_, this->frame_group_.mosi[1]);
  publish(this->frame2_miso_text_sensor_, this->frame_group_.miso[2]);
  publish(this->frame2_mosi_text_sensor_, this->frame_group_.mosi[2]);
  publish(this->frame3_miso_text_sensor_, this->frame_group_.miso[3]);
  publish(this->frame3_mosi_text_sensor_, this->frame_group_.mosi[3]);
  publish(this->frame4_miso_text_sensor_, this->frame_group_.miso[4]);
  publish(this->frame4_mosi_text_sensor_, this->frame_group_.mosi[4]);

  // --- Derived values from validated frame data ---
  // Raw byte from frame 1 MISO has bit 0 always 0; bits [4:1] form the index.
  uint8_t selector_raw = static_cast<uint8_t>(this->frame_group_.miso[1]);
  uint8_t selector_idx = selector_raw >> 1;  // 0–15

  // Selector position (shifted index)
  if (this->selector_position_sensor_ != nullptr) {
    this->selector_position_sensor_->publish_state(selector_idx);
  }

  // Selector position name (mapped from YAML-configured labels)
  if (this->selector_position_name_text_sensor_ != nullptr) {
    if (selector_idx < SELECTOR_LABEL_COUNT && this->selector_labels_[selector_idx] != nullptr) {
      this->selector_position_name_text_sensor_->publish_state(this->selector_labels_[selector_idx]);
    } else {
      // No label configured for this position: publish the index
      snprintf(buf, sizeof(buf), "%u", selector_idx);
      this->selector_position_name_text_sensor_->publish_state(buf);
    }
  }

  // --- Button binary sensors (derived from frame MISO bytes) ---
  uint8_t f3miso_byte = static_cast<uint8_t>(this->frame_group_.miso[3]);
  uint8_t f4miso_byte = static_cast<uint8_t>(this->frame_group_.miso[4]);

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

}  // namespace esphome::whirlpool_spi
