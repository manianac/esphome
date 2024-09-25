#pragma once

#include "../RoombaComponent.h"
#include "esphome/components/sensor/sensor.h"

namespace esphome {
namespace roomba {

class RoombaSensor : public RoombaListener, public Component {
 public:
  void dump_config() override;
  void set_batteryvoltage_sensor(sensor::Sensor *sensor) { this->batteryvoltage_ = sensor; };
  void set_batterycurrent_sensor(sensor::Sensor *sensor) { this->batterycurrent_ = sensor; };
  void set_batterycharge_sensor(sensor::Sensor *sensor) { this->batterycharge_ = sensor; };
  void set_batterycapacity_sensor(sensor::Sensor *sensor) { this->batterycapacity_ = sensor; };
  void set_batterytemperature_sensor(sensor::Sensor *sensor) { this->batterytemperature_ = sensor; };
  void set_dirtsensor_left_sensor(sensor::Sensor *sensor) { this->dirtsensor_left_ = sensor; };
  void set_dirtsensor_right_sensor(sensor::Sensor *sensor) { this->dirtsensor_right_ = sensor; };

  void onNewSensorData6(const sensor_packet6 &response) override;
  void onNoSensorData() override;

 protected:
  sensor::Sensor *batteryvoltage_{nullptr};
  float f_batteryVoltage_;
  sensor::Sensor *batterycurrent_{nullptr};
  float f_batteryCurrent_;
  sensor::Sensor *batterycharge_{nullptr};
  float f_batteryCharge_;
  sensor::Sensor *batterycapacity_{nullptr};
  float f_batteryCapacity;
  sensor::Sensor *batterytemperature_{nullptr};
  int8_t i_battery_Temperature_;
  sensor::Sensor *dirtsensor_left_{nullptr};
  uint8_t i_DirtSensor_Left_;
  sensor::Sensor *dirtsensor_right_{nullptr};
  uint8_t i_DirtSensor_Right_;
};

}  // namespace roomba
}  // namespace esphome
