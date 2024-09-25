#include "RoombaComponent.h"
#include "esphome/core/log.h"

namespace esphome {
namespace roomba {

static const char *const TAG = "RoombaComponent";

void RoombaComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "Roomba Config:");
  // LOG_SENSOR("  ", "Battery Voltage", this->batteryvoltage_);
  // LOG_SENSOR("  ", "Battery Current", this->batterycurrent_);
  // LOG_SENSOR("  ", "Battery Charge", this->batterycharge_);
  // LOG_SENSOR("  ", "Battery Capacity", this->batterycapacity_);
  this->check_uart_settings(115200);
}
/// @brief Reads sensor packet id from Roomba
/// @param destBuffer Destination buffer (must be pre-allocated)
/// @param destBufferLength Size of destination buffer
/// @return True if successful
bool RoombaComponent::readSensorData(uint8_t *destBuffer, const size_t destBufferLength) {
  return this->read_array(destBuffer, destBufferLength);
}

void RoombaComponent::loop() {
  static loopState state = SEND_STARTOI;
  static RoombaListener::sensor_packet6 response{};
  static size_t listenerIndex = 0;
  static uint32_t pastTimestamp = 0;
  uint8_t oi_mode = 0xFF;

  switch (state) {
    case SEND_STARTOI:
      if (millis() - pastTimestamp < 5000)  // Limit "is awake" checks
        break;
      while (this->available())
        this->read();    // Empty serial buffer
      this->write(128);  // Initialize SCI
      state = SEND_SENSOR_REQUEST;
      pastTimestamp = millis();
      break;
    case SEND_SENSOR_REQUEST:
      if (millis() - pastTimestamp < 200)  // To avoid slamming the SCI bus
        break;
      this->write_array({142, (uint8_t) Sensors7to42});
      state = READ_SENSOR_DATA;
      pastTimestamp = millis();
      break;
    case READ_SENSOR_DATA:
      if (millis() - pastTimestamp < 200)  // 200ms for Roomba to process the data packet
        break;
      state = SEND_STARTOI;  // Fallback if reading the serial buffer fails
      if (this->read_array(response.raw, sizeof(response.raw))) {
        state = PROCESS_DATA;
      } else {
        // Send no data on fail to read uart
        for (const auto &listener : listeners_) {
          listener->onNoSensorData();
        }
      }
      break;
    case PROCESS_DATA:
      if (!listeners_.empty()) {
        listeners_[listenerIndex]->onNewSensorData6(response);
        if (++listenerIndex >= listeners_.size()) {
          listenerIndex = 0;
          state = SEND_SENSOR_REQUEST;
        }
      }
      break;
    default:
      state = SEND_STARTOI;
  }
}

}  // namespace roomba
}  // namespace esphome
