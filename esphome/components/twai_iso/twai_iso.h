// ESPHome TWAI ISO-TP Component Header
// File: components/twai_iso/twai_iso.h

#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"
#include "esphome/core/automation.h"
#include <array>
#include <memory>
#include <cstdint>
#include <cstring>

// Include ESP-IDF TWAI only on ESP32 with proper C linkage
#ifdef USE_ESP32
extern "C" {
#include "driver/twai.h"
#include "esp_err.h"
}
#endif

namespace esphome {
namespace twai_iso {

static const char *const TAG = "twai_iso";

/**
 * @brief ESPHome ISO-TP (ISO 14229-2) Component for ESP32
 */
class TWAIISOComponent : public Component {
 public:
  static constexpr uint16_t MAX_MESSAGE_SIZE = 4095;
  static constexpr uint8_t DEFAULT_PADDING_BYTE = 0xAA;
  static constexpr uint8_t CAN_FRAME_SIZE = 8;
  static constexpr uint32_t DEFAULT_TIMEOUT_MS = 1000;

  enum class State : uint8_t { IDLE = 0, SEND_WAIT_FC, SEND_SENDING_CF, RECV_WAIT_CF };

  enum class PciType : uint8_t {
    SINGLE_FRAME = 0x00,
    FIRST_FRAME = 0x01,
    CONSECUTIVE_FRAME = 0x02,
    FLOW_CONTROL = 0x03
  };

  enum class FlowStatus : uint8_t { CONTINUE_TO_SEND = 0x00, WAIT = 0x01, OVERFLOW = 0x02 };

  TWAIISOComponent() = default;

  // ESPHome component lifecycle
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::HARDWARE; }

  // Configuration setters
  void set_tx_pin(InternalGPIOPin *pin) { tx_pin_ = pin; }
  void set_rx_pin(InternalGPIOPin *pin) { rx_pin_ = pin; }
  void set_sleep_pin(InternalGPIOPin *pin) { sleep_pin_ = pin; }
  void set_baudrate(uint32_t baudrate) { baudrate_ = baudrate; }
  void set_buffer_size(uint16_t size) { buffer_size_ = size; }
  void set_padding_byte(uint8_t byte) { padding_byte_ = byte; }
  void set_auto_sleep(bool enable) { auto_sleep_enable_ = enable; }
  void set_auto_sleep_timeout(uint32_t timeout) { auto_sleep_timeout_ms_ = timeout; }

  // Timeout configuration
  void set_timeout_n_as(uint32_t timeout) { timeouts_.n_as_ms = timeout; }
  void set_timeout_n_ar(uint32_t timeout) { timeouts_.n_ar_ms = timeout; }
  void set_timeout_n_bs(uint32_t timeout) { timeouts_.n_bs_ms = timeout; }
  void set_timeout_n_br(uint32_t timeout) { timeouts_.n_br_ms = timeout; }
  void set_timeout_n_cs(uint32_t timeout) { timeouts_.n_cs_ms = timeout; }
  void set_timeout_n_cr(uint32_t timeout) { timeouts_.n_cr_ms = timeout; }

  // Flow control configuration
  void set_flow_control_block_size(uint8_t size) { fc_config_.block_size = size; }
  void set_flow_control_st_min(uint8_t st_min) { fc_config_.st_min_ms = st_min; }

  // Public API methods
  bool send_message(uint32_t tx_id, uint32_t expected_response_id, const uint8_t *payload, uint16_t length);

#ifdef USE_ESP32
  bool process_can_message(const twai_message_t &can_msg, uint16_t &out_message_length,
                           const uint8_t *&out_message_buffer);
#endif

  void reset_link();
  bool is_busy() const { return is_session_active_; }
  State get_state() const { return state_; }
  bool enable_transceiver();
  bool disable_transceiver();
  bool is_transceiver_enabled() const { return transceiver_enabled_; }

  // Callback support
  void add_on_message_callback(std::function<void(uint32_t, const std::vector<uint8_t> &)> callback) {
    on_message_callbacks_.push_back(std::move(callback));
  }

 protected:
  // Configuration
  InternalGPIOPin *tx_pin_{nullptr};
  InternalGPIOPin *rx_pin_{nullptr};
  InternalGPIOPin *sleep_pin_{nullptr};
  uint32_t baudrate_{500000};
  uint16_t buffer_size_{4095};
  uint8_t padding_byte_{DEFAULT_PADDING_BYTE};
  bool auto_sleep_enable_{false};
  uint32_t auto_sleep_timeout_ms_{30000};

  // Timeout configuration structure
  struct TimeoutConfig {
    uint32_t n_as_ms{DEFAULT_TIMEOUT_MS};
    uint32_t n_ar_ms{DEFAULT_TIMEOUT_MS};
    uint32_t n_bs_ms{DEFAULT_TIMEOUT_MS};
    uint32_t n_br_ms{DEFAULT_TIMEOUT_MS};
    uint32_t n_cs_ms{0};
    uint32_t n_cr_ms{DEFAULT_TIMEOUT_MS};
  } timeouts_;

  // Flow Control configuration structure
  struct FlowControlConfig {
    uint8_t block_size{0};
    uint8_t st_min_ms{0};
  } fc_config_;

  // Runtime state
  State state_{State::IDLE};
  bool is_session_active_{false};
  uint32_t local_tx_can_id_{0};
  uint32_t remote_rx_can_id_{0};
  bool twai_initialized_{false};
  bool transceiver_enabled_{false};

  // Transmission/Reception state
  const uint8_t *send_buffer_ptr_{nullptr};
  uint16_t send_total_length_{0};
  uint16_t send_offset_{0};
  uint8_t send_sequence_number_{0};
  uint8_t fc_block_size_received_{0};
  uint8_t fc_st_min_ms_received_{0};
  uint8_t flow_control_cf_to_send_in_block_{0};
  std::vector<uint8_t> receive_buffer_;
  uint16_t receive_total_expected_length_{0};
  uint16_t receive_offset_{0};
  uint8_t receive_sequence_number_expected_{0};

  // Timing
  uint32_t last_action_timestamp_ms_{0};
  uint32_t last_activity_timestamp_ms_{0};

  // Callbacks
  std::vector<std::function<void(uint32_t, const std::vector<uint8_t> &)>> on_message_callbacks_;

  // Internal methods (ESP32-specific ones guarded)
#ifdef USE_ESP32
  bool send_can_frame(uint32_t id, const uint8_t *data, uint8_t dlc);
  bool handle_single_frame(const twai_message_t &can_msg, uint16_t &out_message_length,
                           const uint8_t *&out_message_buffer);
  bool handle_first_frame(const twai_message_t &can_msg);
  bool handle_consecutive_frame(const twai_message_t &can_msg, uint16_t &out_message_length,
                                const uint8_t *&out_message_buffer);
  bool handle_flow_control(const twai_message_t &can_msg);
  bool initialize_twai();
  void deinitialize_twai();
#endif

  void reset_internal();
  bool send_flow_control(FlowStatus status);
  bool should_process_message(uint32_t can_id);
  void setup_new_session(uint32_t rx_can_id);
  void handle_timeouts();
  void continue_sending_consecutive_frames();
  void validate_vin_data(const uint8_t *data, uint16_t length);
  bool init_transceiver_sleep_pin();
  void handle_auto_sleep();
  void update_activity_timestamp();
  void call_message_callbacks(uint32_t can_id, const std::vector<uint8_t> &data);
};

}  // namespace twai_iso
}  // namespace esphome
