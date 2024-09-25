#include "RoombaSensor.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace esphome {
namespace roomba {

static const char *const TAG = "Roomba.text_sensor";
static __attribute__((always_inline)) inline float recipsf2(float a) {
  float result;
  asm volatile("wfr f1, %1\n"

               "recip0.s f0, f1\n"
               "const.s f2, 1\n"
               "msub.s f2, f1, f0\n"
               "maddn.s f0, f0, f2\n"
               "const.s f2, 1\n"
               "msub.s f2, f1, f0\n"
               "maddn.s f0, f0, f2\n"

               "rfr %0, f0\n"
               : "=r"(result)
               : "r"(a)
               : "f0", "f1", "f2");
  return result;
}
#define DIV(a, b) (a) * recipsf2(b)

void RoombaSensor::dump_config() {
  ESP_LOGCONFIG(TAG, "Roomba Sensor:");
  LOG_SENSOR("  ", "Battery Voltage", this->batteryvoltage_);
  LOG_SENSOR("  ", "Battery Current", this->batterycurrent_);
  LOG_SENSOR("  ", "Battery Charge", this->batterycharge_);
  LOG_SENSOR("  ", "Battery Capacity", this->batterycapacity_);
  LOG_SENSOR("  ", "Battery Temperature", this->batterytemperature_);
  LOG_SENSOR("  ", "Dirt Sensor Left", this->dirtsensor_left_);
  LOG_SENSOR("  ", "Dirt Sensor Right", this->dirtsensor_right_);
}

void RoombaSensor::onNoSensorData() {
  ESP_LOGW(TAG, "Reading data from Roomba failed!");
  status_set_warning();
  if (this->batteryvoltage_ != nullptr)
    this->batteryvoltage_->publish_state(NAN);
  if (this->batterycurrent_ != nullptr)
    this->batterycurrent_->publish_state(NAN);
  if (this->batterycharge_ != nullptr)
    this->batterycharge_->publish_state(NAN);
  if (this->batterycapacity_ != nullptr)
    this->batterycapacity_->publish_state(NAN);
  if (this->batterytemperature_ != nullptr)
    this->batterytemperature_->publish_state(NAN);
  if (this->dirtsensor_left_ != nullptr)
    this->dirtsensor_left_->publish_state(NAN);
  if (this->dirtsensor_right_ != nullptr)
    this->dirtsensor_right_->publish_state(NAN);
}

void RoombaSensor::onNewSensorData6(const sensor_packet6 &response) {
  ESP_LOGV(TAG, "Processing RoombaSensor::onNewData");
  this->status_clear_warning();
  if (this->batteryvoltage_ != nullptr) {
    const float newVoltage = DIV(__ntohs(response.reg.voltage), 1000);
    if (f_batteryVoltage_ != newVoltage) {
      f_batteryVoltage_ = newVoltage;
      this->batteryvoltage_->publish_state(newVoltage);
    }
  }
  if (this->batterycurrent_ != nullptr) {
    const int16_t batteryCurrent = __ntohs(response.reg.current);  // Keep this to show sign
    const float newCurrent = DIV(batteryCurrent, 1000);
    if (f_batteryCurrent_ != newCurrent) {
      f_batteryCurrent_ = newCurrent;
      this->batterycurrent_->publish_state(newCurrent);
    }
  }

  if (this->batterycharge_ != nullptr) {
    const float newCharge = DIV(__ntohs(response.reg.charge), 1000);
    if (f_batteryCharge_ != newCharge) {
      f_batteryCharge_ = newCharge;
      this->batterycharge_->publish_state(f_batteryCharge_);
    }
  }

  if (this->batterycapacity_ != nullptr) {
    float newCapacity = DIV(__ntohs(response.reg.capacity), 1000);
    if (f_batteryCapacity != newCapacity) {
      f_batteryCapacity = newCapacity;
      this->batterycapacity_->publish_state(newCapacity);
    }
  }

  if (this->batterytemperature_ != nullptr) {
    const int8_t newTemperature = response.reg.temperature;
    if (i_battery_Temperature_ != newTemperature) {
      i_battery_Temperature_ = newTemperature;
      this->batterytemperature_->publish_state(newTemperature);
    }
  }

  if (this->dirtsensor_left_ != nullptr) {
    const uint8_t newDirtSensorLeft = response.reg.dirtdetector_left;
    if (i_DirtSensor_Left_ != newDirtSensorLeft) {
      i_DirtSensor_Left_ = newDirtSensorLeft;
      this->dirtsensor_left_->publish_state(newDirtSensorLeft);
    }
  }

  if (this->dirtsensor_right_ != nullptr) {
    const uint8_t newDirtSensorRight = response.reg.dirtdetector_right;
    if (i_DirtSensor_Right_ != newDirtSensorRight) {
      i_DirtSensor_Right_ = newDirtSensorRight;
      this->dirtsensor_right_->publish_state(newDirtSensorRight);
    }
  }
}

}  // namespace roomba
}  // namespace esphome
