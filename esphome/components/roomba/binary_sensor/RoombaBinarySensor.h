#pragma once

#include "../RoombaComponent.h"
#include "esphome/components/binary_sensor/binary_sensor.h"

namespace esphome {
namespace roomba {

class RoombaBinarySensor : public RoombaListener, public Component {
 public:
  void dump_config() override;

  void set_leftbump_sensor(binary_sensor::BinarySensor *sensor) { this->bump_left_binarysensor_ = sensor; };
  void set_rightbump_sensor(binary_sensor::BinarySensor *sensor) { this->bump_right_binarysensor_ = sensor; };
  void set_leftwheeldrop_sensor(binary_sensor::BinarySensor *sensor) { this->wheeldrop_left_binarysensor_ = sensor; };
  void set_rightwheeldrop_sensor(binary_sensor::BinarySensor *sensor) { this->wheeldrop_right_binarysensor_ = sensor; };
  void set_casterwheeldrop_sensor(binary_sensor::BinarySensor *sensor) {
    this->wheeldrop_caster_binarysensor_ = sensor;
  };
  void set_wall_sensor(binary_sensor::BinarySensor *sensor) { this->wall_binarysensor_ = sensor; };
  void set_cliffleft_sensor(binary_sensor::BinarySensor *sensor) { this->clift_left_binarysensor_ = sensor; };
  void set_cliffleftfront_sensor(binary_sensor::BinarySensor *sensor) { this->clift_frontleft_binarysensor_ = sensor; };
  void set_cliffrightfront_sensor(binary_sensor::BinarySensor *sensor) {
    this->clift_frontright_binarysensor_ = sensor;
  };
  void set_cliffright_sensor(binary_sensor::BinarySensor *sensor) { this->clift_right_binarysensor_ = sensor; };
  void set_virtualwall_sensor(binary_sensor::BinarySensor *sensor) { this->virtualwall_binarysensor_ = sensor; };
  void set_sidebrush_overcurrent_sensor(binary_sensor::BinarySensor *sensor) {
    this->motor_sidebrush_overcurrent_binarysensor_ = sensor;
  };
  void set_vacuum_overcurrent_sensor(binary_sensor::BinarySensor *sensor) {
    this->motor_vacuum_overcurrent_binarysensor_ = sensor;
  };
  void set_mainbrush_overcurrent_sensor(binary_sensor::BinarySensor *sensor) {
    this->motor_mainbrush_overcurrent_binarysensor_ = sensor;
  };
  void set_rightwheel_overcurrent_sensor(binary_sensor::BinarySensor *sensor) {
    this->motor_rightwheel_overcurrent_binarysensor_ = sensor;
  };
  void set_leftwheel_overcurrent_sensor(binary_sensor::BinarySensor *sensor) {
    this->motor_leftwheel_overcurrent_binarysensor_ = sensor;
  };
  void set_powerbutton_sensor(binary_sensor::BinarySensor *sensor) { this->powerbutton_binarysensor_ = sensor; };
  void set_spotbutton_sensor(binary_sensor::BinarySensor *sensor) { this->spotbutton_binarysensor_ = sensor; };
  void set_cleanbutton_sensor(binary_sensor::BinarySensor *sensor) { this->cleanbutton_binarysensor_ = sensor; };

  void onNewSensorData6(const sensor_packet6 &response) override;

 protected:
  union roomba_sensor1 {
    struct __attribute__((__packed__)) {
      uint8_t bumps_wheeldrops;
      uint8_t wall;
      uint8_t cliff_left;
      uint8_t cliff_frontleft;
      uint8_t cliff_frontright;
      uint8_t cliff_right;
      uint8_t virtual_wall;
      uint8_t motor_overcurrents;
      uint8_t dirtdetector_left;
      uint8_t dirtdetector_right;
    } reg;
    mutable uint8_t raw[sizeof(reg)];
  };
  binary_sensor::BinarySensor *bump_left_binarysensor_{nullptr};
  binary_sensor::BinarySensor *bump_right_binarysensor_{nullptr};
  binary_sensor::BinarySensor *wheeldrop_left_binarysensor_{nullptr};
  binary_sensor::BinarySensor *wheeldrop_right_binarysensor_{nullptr};
  binary_sensor::BinarySensor *wheeldrop_caster_binarysensor_{nullptr};
  binary_sensor::BinarySensor *wall_binarysensor_{nullptr};
  binary_sensor::BinarySensor *clift_left_binarysensor_{nullptr};
  binary_sensor::BinarySensor *clift_frontleft_binarysensor_{nullptr};
  binary_sensor::BinarySensor *clift_frontright_binarysensor_{nullptr};
  binary_sensor::BinarySensor *clift_right_binarysensor_{nullptr};
  binary_sensor::BinarySensor *virtualwall_binarysensor_{nullptr};
  binary_sensor::BinarySensor *motor_sidebrush_overcurrent_binarysensor_{nullptr};
  binary_sensor::BinarySensor *motor_vacuum_overcurrent_binarysensor_{nullptr};
  binary_sensor::BinarySensor *motor_mainbrush_overcurrent_binarysensor_{nullptr};
  binary_sensor::BinarySensor *motor_rightwheel_overcurrent_binarysensor_{nullptr};
  binary_sensor::BinarySensor *motor_leftwheel_overcurrent_binarysensor_{nullptr};
  binary_sensor::BinarySensor *powerbutton_binarysensor_{nullptr};
  binary_sensor::BinarySensor *spotbutton_binarysensor_{nullptr};
  binary_sensor::BinarySensor *cleanbutton_binarysensor_{nullptr};
};

}  // namespace roomba
}  // namespace esphome
