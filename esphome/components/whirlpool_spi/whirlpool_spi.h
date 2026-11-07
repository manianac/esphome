#pragma once

#include "esphome/core/component.h"
#include "esphome/core/defines.h"
#include "esphome/core/gpio.h"

#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"

#include <atomic>

#ifdef USE_WHIRLPOOL_SPI_PERIPHERAL
#include "driver/spi_slave.h"  // NOLINT
#else
#include "driver/pulse_cnt.h"
#endif

namespace esphome::whirlpool_spi {

static constexpr uint32_t MAX_RECORDS = 64;  // Transfers the ring buffer holds; must be a power of two
static_assert((MAX_RECORDS & (MAX_RECORDS - 1)) == 0, "MAX_RECORDS must be a power of two");
static constexpr uint32_t RING_MASK = MAX_RECORDS - 1;
static constexpr uint32_t EXPECTED_BITS = 32;        // Bits per transfer on the washer's bus
static constexpr uint32_t FRAME_GROUP_SIZE = 5;      // 1 marker + 4 data frames per cycle
static constexpr uint32_t MARKER_MISO = 0x94949494;  // MISO value of the marker frame
static constexpr uint32_t DEBOUNCE_DEPTH = 3;        // Consecutive identical groups required before publishing
static constexpr uint8_t SELECTOR_LABEL_COUNT = 16;  // Max selector positions (0-15)

// Button bits in the low byte of a frame's MISO word
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

/// One captured transfer. Data is MSB first: the first bit on the wire is bit 31.
struct Transfer {
  uint32_t bit_count;
  uint32_t data_miso;
  uint32_t data_mosi;
};

/// One complete marker-plus-data cycle, compared as a unit for debounce and change detection.
struct FrameGroup {
  uint32_t miso[FRAME_GROUP_SIZE];
  uint32_t mosi[FRAME_GROUP_SIZE];
  bool operator==(const FrameGroup &other) const = default;
};

#ifdef USE_WHIRLPOOL_SPI_PERIPHERAL
class WhirlpoolSPI;

/// One SPI peripheral receiving one of the bus's two data lines.
struct CaptureChannel {
  WhirlpoolSPI *parent{nullptr};
  spi_host_device_t host{};
  spi_slave_transaction_t transaction{};  // NOLINT
  uint32_t rx_buffer[2]{};                // Longer than a frame, so that over-length frames are still counted
  uint32_t data{0};                       // Last completed frame, MSB first
  uint32_t bit_count{0};                  // Bits in the last completed frame
  uint32_t end_inputs{0};                 // GPIO_IN_REG when the last frame completed
  uint32_t completed_at{0};               // CPU cycle count when the last frame completed
  uint8_t data_bit{0};                    // Data line's GPIO number, which is also its bit in GPIO_IN_REG
  bool pending{false};                    // Holding a completed frame until the other channel completes the same one
};
#endif

class WhirlpoolSPI : public PollingComponent {
 public:
  void set_miso_pin(InternalGPIOPin *pin) { this->miso_pin_ = pin; }
  void set_mosi_pin(InternalGPIOPin *pin) { this->mosi_pin_ = pin; }
  void set_cs_pin(InternalGPIOPin *pin) { this->cs_pin_ = pin; }
  void set_clk_pin(InternalGPIOPin *pin) { this->clk_pin_ = pin; }

  /// Set a selector position label (index 0-15, from YAML). The string must outlive the component.
  void set_selector_label(uint8_t index, const char *label) {
    if (index < SELECTOR_LABEL_COUNT)
      this->selector_labels_[index] = label;
  }

  void setup() override;
  void dump_config() override;
  void loop() override;
  void update() override;
  float get_setup_priority() const override { return setup_priority::IO; }

  // Raw words of each frame in the group, as hex
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
  SUB_TEXT_SENSOR(selector_position_name)

  SUB_SENSOR(selector_position)

  // Diagnostics, published every update_interval
  SUB_SENSOR(transfers_per_sec)
  SUB_SENSOR(buffer_queue)
  SUB_SENSOR(buffer_overflows)
  SUB_SENSOR(buffer_peak)
  SUB_SENSOR(valid_frames)
  SUB_SENSOR(error_frames)
  SUB_SENSOR(error_under)
  SUB_SENSOR(error_over)
  SUB_SENSOR(groups_completed)
  SUB_SENSOR(groups_dropped)
  SUB_SENSOR(groups_failed_plausibility)

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

 protected:
#ifdef USE_WHIRLPOOL_SPI_PERIPHERAL
  /// Runs in the driver's ISR each time CS rises on one channel.
  static void channel_transfer_done_(spi_slave_transaction_t *trans);  // NOLINT
  /// Bring up one SPI peripheral receiving data_pin. Returns false if the driver refuses.
  bool start_capture_channel_(CaptureChannel &channel, spi_host_device_t host, InternalGPIOPin *data_pin);
#else
  static void cs_isr_(WhirlpoolSPI *c);
  static void clk_isr_(WhirlpoolSPI *c);
  /// Count CLK falling edges through the pulse counter's glitch filter. Returns false if the driver refuses.
  bool start_clk_counter_();
#endif
  /// Producer end of the ring buffer, called from the capture ISRs.
  void push_transfer_(uint32_t bit_count, uint32_t miso, uint32_t mosi);

  /// Run one captured transfer through the frame-group state machine.
  void process_frame_(const Transfer &frame);
  /// Validate, debounce and publish the group that was just completed.
  void process_complete_group_();
  /// Publish the completed frame group to the sensors.
  void publish_frame_group_();
  /// Every MISO frame must be one byte repeated four times. Returns true if the group passes.
  bool validate_frame_group_();

  InternalGPIOPin *miso_pin_{nullptr};
  InternalGPIOPin *mosi_pin_{nullptr};
  InternalGPIOPin *cs_pin_{nullptr};
  InternalGPIOPin *clk_pin_{nullptr};

#ifdef USE_WHIRLPOOL_SPI_PERIPHERAL
  // Capture state, touched only by the transfer-done callback
  CaptureChannel miso_channel_{};
  CaptureChannel mosi_channel_{};
  std::atomic<uint32_t> channel_desyncs_{0};  // Frames only one channel saw, since boot
#else
  // Capture state, touched only by the ISRs
  pcnt_unit_handle_t clk_counter_{nullptr};  // Counts CLK falling edges longer than CLK_GLITCH_FILTER_NS
  int last_clk_count_{0};                    // clk_counter_ at the last accepted falling edge
  uint32_t bit_count_{0};                    // Accepted falling edges in the current transfer
  uint32_t miso_shift_{0};                   // MISO bits of the current transfer
  uint32_t mosi_shift_{0};                   // MOSI bits of the current transfer
  uint8_t miso_bit_{0};                      // Bit position of each pin in GPIO_IN_REG
  uint8_t mosi_bit_{0};
  std::atomic<uint32_t> clk_glitches_{0};  // CLK interrupts the pulse counter didn't confirm, since boot
#endif
  uint8_t cs_bit_{0};  // CS pin's bit position in GPIO_IN_REG

  // Ring buffer of completed transfers. Single producer (ISR) and single consumer (loop), so it needs no lock:
  // only the ISR writes ring_head_ and only loop() writes ring_tail_. Both count up freely and wrap via RING_MASK.
  Transfer transfer_buffer_[MAX_RECORDS];
  std::atomic<uint32_t> ring_head_{0};
  std::atomic<uint32_t> ring_tail_{0};
  std::atomic<uint32_t> ring_overflows_{0};  // Transfers dropped because the buffer was full, since boot
  std::atomic<uint32_t> ring_peak_{0};       // Most transfers queued at once since the last update()

  // Counters, reset each update()
  uint32_t error_frames_{0};        // Frames with unexpected bit counts
  uint32_t error_under_{0};         // Frames with fewer bits than expected
  uint32_t error_over_{0};          // Frames with more bits than expected
  uint32_t accum_processed_{0};     // Transfers processed
  uint32_t accum_valid_frames_{0};  // Valid frames
  uint32_t last_buffer_queue_{0};   // Buffer queue depth at last loop() snapshot
  uint32_t last_status_log_{0};     // When the buffer accumulation warning last fired

  // Frame-group state machine: collects FRAME_GROUP_SIZE consecutive valid frames starting from a marker.
  // If any frame is dropped (wrong bit count), the entire group is discarded.
  FrameGroup frame_group_{};
  uint32_t frame_group_pos_{0};             // Next expected position (0 = waiting for marker)
  uint32_t groups_completed_{0};            // Successfully completed groups
  uint32_t groups_dropped_{0};              // Groups dropped due to missing/bad frames
  uint32_t groups_failed_plausibility_{0};  // Groups dropped due to plausibility check failure

  // Debounce: a group is accepted once DEBOUNCE_DEPTH valid groups in a row are identical,
  // and only published when it differs from what was last published.
  FrameGroup candidate_group_{};
  FrameGroup published_group_{};
  uint8_t match_count_{0};  // Consecutive groups equal to candidate_group_
  bool has_published_{false};

  // Selector position labels (index 0-15) from YAML; nullptr means no label for that position.
  const char *selector_labels_[SELECTOR_LABEL_COUNT]{};
};

}  // namespace esphome::whirlpool_spi
