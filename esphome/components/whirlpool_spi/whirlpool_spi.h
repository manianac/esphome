#pragma once

#include "esphome/core/component.h"
#include "esphome/core/gpio.h"
#include "esphome/core/log.h"

#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"

#include <string>

namespace esphome {
namespace whirlpool_spi {

static const char *const TAG = "whirlpool_spi";
static constexpr uint32_t MAX_RECORDS = 64;                // Max number of transfers to buffer
static constexpr uint32_t FRAME_GROUP_SIZE = 5;            // 1 marker + 4 data frames per cycle
static constexpr uint32_t MARKER_MISO_UPPER = 0x94949494;  // Upper 32 bits of MISO marker frame
static constexpr uint8_t SELECTOR_LABEL_COUNT = 16;        // Max selector positions (0–15)

// --- Button bit-masks (frame MISO upper byte) ---
// Known masks (Frame 3):
static constexpr uint8_t BUTTON_MASK_CYCLE_SIGNAL = 0x04;  // Frame 3
static constexpr uint8_t BUTTON_MASK_START_PAUSE = 0x10;   // Frame 3
static constexpr uint8_t BUTTON_MASK_POWER = 0x20;         // Frame 3
static constexpr uint8_t BUTTON_MASK_DELAY_WASH = 0x40;    // Frame 3
static constexpr uint8_t BUTTON_MASK_ECOBOOST = 0x01;      // Frame 4
static constexpr uint8_t BUTTON_MASK_EXTRA_RINSE = 0x40;   // Frame 4
static constexpr uint8_t BUTTON_MASK_DEEP_CLEAN = 0x80;    // Frame 4
static constexpr uint8_t BUTTON_MASK_SOIL_LEVEL = 0x02;    // Frame 4
static constexpr uint8_t BUTTON_MASK_SPIN_SPEED = 0x04;    // Frame 4
static constexpr uint8_t BUTTON_MASK_WASH_TEMP = 0x08;     // Frame 4
static constexpr uint8_t BUTTON_MASK_DISPENSERS = 0x10;    // Frame 4

struct Transfer {
  uint32_t bit_count;
  uint64_t data_miso;
  uint64_t data_mosi;
};

class WhirlpoolSPI : public PollingComponent {
 public:
  void set_miso_pin(InternalGPIOPin *pin) { this->miso_pin_ = pin; }
  void set_mosi_pin(InternalGPIOPin *pin) { this->mosi_pin_ = pin; }
  void set_cs_pin(InternalGPIOPin *pin) { this->cs_pin_ = pin; }
  void set_clk_pin(InternalGPIOPin *pin) { this->clk_pin_ = pin; }

  /// Set a selector position label (index 0–15, populated from YAML).
  void set_selector_label(uint8_t index, const std::string &label) {
    if (index < SELECTOR_LABEL_COUNT)
      this->selector_labels_[index] = label;
  }

  void setup() override;
  void dump_config() override;
  void loop() override;
  void update() override;
  float get_setup_priority() const override { return setup_priority::IO; }

  /// Publish completed frame group to text sensors.
  void publish_frame_group_();

  /// Run plausibility checks on a completed frame group.
  /// Returns true if the group passes all checks.
  bool validate_frame_group_();

  /// Compare two frame groups for equality (MISO and MOSI data fields).
  static bool frame_groups_match_(const Transfer *a, const Transfer *b);

  // Public members for ISR access
  InternalGPIOPin *miso_pin_ = nullptr;
  InternalGPIOPin *mosi_pin_ = nullptr;
  InternalGPIOPin *cs_pin_ = nullptr;
  InternalGPIOPin *clk_pin_ = nullptr;

  // State tracking
  uint32_t bit_count_{0};              // Number of bits in current transfer (working)
  uint64_t data_miso_{0};              // MISO data captured (working)
  uint64_t data_mosi_{0};              // MOSI data captured (working)
  uint32_t expected_bits_{32};         // Expected number of bits per transfer (configurable)
  bool cs_active_{false};              // Set by CS ISR when CS is low
  uint32_t buffer_size_{MAX_RECORDS};  // Ring buffer size (configurable, defaults to max)

  // Ring buffer for completed transfers
  Transfer transfer_buffer_[MAX_RECORDS];
  uint32_t write_idx_{0};     // Where ISR writes next
  uint32_t read_idx_{0};      // Where loop() reads next
  uint32_t buffer_count_{0};  // How many records are buffered

  // Counters
  uint32_t error_frames_{0};  // Frames with unexpected bit counts
  uint32_t error_under_{0};   // Frames with fewer bits than expected
  uint32_t error_over_{0};    // Frames with more bits than expected

  // Accumulators for diagnostic publishing (reset each update() cycle)
  uint32_t accum_processed_{0};     // Transfers processed since last update()
  uint32_t accum_valid_frames_{0};  // Valid frames since last update()
  uint32_t last_buffer_queue_{0};   // Buffer queue depth at last loop() snapshot

  // ISR-safe pin access (no virtual calls in interrupt context)
  ISRInternalGPIOPin miso_isr_{};
  ISRInternalGPIOPin mosi_isr_{};
  ISRInternalGPIOPin cs_isr_{};

  // Initialization flag to prevent ISRs from firing before setup is complete
  volatile bool initialized_{false};

  // --- Frame-group state machine ---
  // Accumulates FRAME_GROUP_SIZE consecutive valid frames starting from a marker.
  // If any frame is dropped (wrong bit count), the entire group is discarded.
  Transfer frame_group_[FRAME_GROUP_SIZE];
  uint32_t frame_group_pos_{0};             // Next expected position (0 = waiting for marker)
  bool frame_group_synced_{false};          // True once we've seen the first marker
  uint32_t groups_completed_{0};            // Successfully completed groups
  uint32_t groups_dropped_{0};              // Groups dropped due to missing/bad frames
  uint32_t groups_failed_plausibility_{0};  // Groups dropped due to plausibility check failure

  // --- Consecutive-match debounce ---
  // Each validated group is written into the next history slot.
  // Publishing only occurs once all slots are filled and all match.
  static constexpr uint32_t DEBOUNCE_DEPTH = 3;
  Transfer frame_group_history_[DEBOUNCE_DEPTH][FRAME_GROUP_SIZE]{};
  uint32_t history_write_idx_{0};  // Next slot to write into (0 to DEBOUNCE_DEPTH-1)
  uint32_t history_count_{0};      // Slots filled so far (0 to DEBOUNCE_DEPTH)

  // Text sensors for each frame position in the group (MISO and MOSI)
  SUB_TEXT_SENSOR(frame0_miso)
  SUB_TEXT_SENSOR(frame0_mosi)
  SUB_TEXT_SENSOR(frame1_miso)
  SUB_TEXT_SENSOR(frame1_mosi)
  SUB_TEXT_SENSOR(frame2_miso)
  SUB_TEXT_SENSOR(frame2_mosi)
  SUB_TEXT_SENSOR(frame3_miso)
  SUB_TEXT_SENSOR(frame3_mosi)
  SUB_TEXT_SENSOR(frame4_miso)
  SUB_TEXT_SENSOR(frame4_mosi)

  // Numeric sensors derived from validated frame data
  SUB_SENSOR(selector_position)

  // Internal diagnostic sensors (published every 10 s)
  SUB_SENSOR(transfers_per_sec)
  SUB_SENSOR(buffer_queue)
  SUB_SENSOR(valid_frames)
  SUB_SENSOR(error_frames)
  SUB_SENSOR(error_under)
  SUB_SENSOR(error_over)
  SUB_SENSOR(groups_completed)
  SUB_SENSOR(groups_dropped)
  SUB_SENSOR(groups_failed_plausibility)

  // Derived text sensors
  SUB_TEXT_SENSOR(selector_position_name)

  // Selector position labels (index 0–15), populated from YAML config.
  // Empty string means no label configured for that position.
  std::string selector_labels_[SELECTOR_LABEL_COUNT];

  // Binary sensors for button states
  SUB_BINARY_SENSOR(power_button)
  SUB_BINARY_SENSOR(delay_wash)
  SUB_BINARY_SENSOR(start_pause)
  SUB_BINARY_SENSOR(cycle_signal)
  SUB_BINARY_SENSOR(ecoboost)
  SUB_BINARY_SENSOR(extra_rinse)
  SUB_BINARY_SENSOR(deep_clean)
  SUB_BINARY_SENSOR(soil_level)
  SUB_BINARY_SENSOR(spin_speed)
  SUB_BINARY_SENSOR(wash_temp)
  SUB_BINARY_SENSOR(dispensers)
};

}  // namespace whirlpool_spi
}  // namespace esphome
