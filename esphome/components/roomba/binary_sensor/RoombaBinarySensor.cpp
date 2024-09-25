#include "RoombaBinarySensor.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace esphome {
namespace roomba {

/// Masks for bumps and wheedrops sensor packet id 7
#define ROOMBA_MASK_BUMP_RIGHT 0x1
#define ROOMBA_MASK_BUMP_LEFT 0x2
#define ROOMBA_MASK_WHEELDROP_RIGHT 0x4
#define ROOMBA_MASK_WHEELDROP_LEFT 0x8
#define ROOMBA_MASK_WHEELDROP_CASTER 0x10

#define ROOMBA_MASK_SIDE_BRUSH 0x1
#define ROOMBA_MASK_VACUUM 0x2
#define ROOMBA_MASK_MAIN_BRUSH 0x4
#define ROOMBA_MASK_RIGHT_WHEEL 0x8
#define ROOMBA_MASK_LEFT_WHEEL 0x10

#define ROOMBA_MASK_BUTTON_CLEAN 0x1
#define ROOMBA_MASK_BUTTON_SPOT 0x2
#define ROOMBA_MASK_BUTTON_DOCK 0x4
#define ROOMBA_MASK_BUTTON_MINUTE 0x8
#define ROOMBA_MASK_BUTTON_HOUR 0x10
#define ROOMBA_MASK_BUTTON_DAY 0x20
#define ROOMBA_MASK_BUTTON_SCHEDULE 0x40
#define ROOMBA_MASK_BUTTON_CLOCK 0x80

static const char *const TAG = "Roomba.binary_sensor";

void RoombaBinarySensor::dump_config() {
  ESP_LOGCONFIG(TAG, "Roomba TextSensor:");
  LOG_BINARY_SENSOR("  ", "Wall Sensor", this->wall_binarysensor_);
  LOG_BINARY_SENSOR("  ", "Left Bump Sensor", this->bump_left_binarysensor_);
  LOG_BINARY_SENSOR("  ", "Right Bump Sensor", this->bump_right_binarysensor_);
  LOG_BINARY_SENSOR("  ", "Left Wheeldrop Sensor", this->wheeldrop_left_binarysensor_);
  LOG_BINARY_SENSOR("  ", "Right Wheeldrop Sensor", this->wheeldrop_right_binarysensor_);
  LOG_BINARY_SENSOR("  ", "Caster Wheeldrop Sensor", this->wheeldrop_caster_binarysensor_);
  LOG_BINARY_SENSOR("  ", "Left Cliff Sensor", this->clift_left_binarysensor_);
  LOG_BINARY_SENSOR("  ", "Left Front Cliff Sensor", this->clift_frontleft_binarysensor_);
  LOG_BINARY_SENSOR("  ", "Right Front Cliff Sensor", this->clift_frontright_binarysensor_);
  LOG_BINARY_SENSOR("  ", "Right Cliff Sensor", this->clift_right_binarysensor_);
  LOG_BINARY_SENSOR("  ", "Virtual Wall Sensor", this->virtualwall_binarysensor_);
  LOG_BINARY_SENSOR("  ", "Side Brush Overcurrent Sensor", this->motor_sidebrush_overcurrent_binarysensor_);
  LOG_BINARY_SENSOR("  ", "Vacuum Overcurrent Sensor", this->motor_vacuum_overcurrent_binarysensor_);
  LOG_BINARY_SENSOR("  ", "Main Brush Overcurrent Sensor", this->motor_mainbrush_overcurrent_binarysensor_);
  LOG_BINARY_SENSOR("  ", "Right Wheel Overcurrent Sensor", this->motor_rightwheel_overcurrent_binarysensor_);
  LOG_BINARY_SENSOR("  ", "Left Wheel Overcurrent Sensor", this->motor_leftwheel_overcurrent_binarysensor_);
  LOG_BINARY_SENSOR("  ", "Power Button Sensor", this->powerbutton_binarysensor_);
  LOG_BINARY_SENSOR("  ", "Spot Button Sensor", this->spotbutton_binarysensor_);
  LOG_BINARY_SENSOR("  ", "Clean Button Sensor", this->cleanbutton_binarysensor_);
}

void RoombaBinarySensor::onNewSensorData6(const sensor_packet6 &response) {
  ESP_LOGV(TAG, "Processing RoombaBinarySensor::onNewData");
  if (false) {  // TODO: Fill in
    ESP_LOGW(TAG, "Reading data from Roomba failed!");
    status_set_warning();
    if (this->wall_binarysensor_ != nullptr)
      this->wall_binarysensor_->publish_state(0);
    if (this->bump_left_binarysensor_ != nullptr)
      this->bump_left_binarysensor_->publish_state(0);
    if (this->bump_right_binarysensor_ != nullptr)
      this->bump_right_binarysensor_->publish_state(0);
    if (this->wheeldrop_left_binarysensor_ != nullptr)
      this->wheeldrop_left_binarysensor_->publish_state(0);
    if (this->wheeldrop_right_binarysensor_ != nullptr)
      this->wheeldrop_right_binarysensor_->publish_state(0);
    if (this->wheeldrop_caster_binarysensor_ != nullptr)
      this->wheeldrop_caster_binarysensor_->publish_state(0);
    if (this->clift_left_binarysensor_ != nullptr)
      this->clift_left_binarysensor_->publish_state(0);
    if (this->clift_frontleft_binarysensor_ != nullptr)
      this->clift_frontleft_binarysensor_->publish_state(0);
    if (this->clift_frontright_binarysensor_ != nullptr)
      this->clift_frontright_binarysensor_->publish_state(0);
    if (this->clift_right_binarysensor_ != nullptr)
      this->clift_right_binarysensor_->publish_state(0);
    if (this->virtualwall_binarysensor_ != nullptr)
      this->virtualwall_binarysensor_->publish_state(0);
    if (this->motor_sidebrush_overcurrent_binarysensor_ != nullptr)
      this->motor_sidebrush_overcurrent_binarysensor_->publish_state(0);
    if (this->motor_vacuum_overcurrent_binarysensor_ != nullptr)
      this->motor_vacuum_overcurrent_binarysensor_->publish_state(0);
    if (this->motor_mainbrush_overcurrent_binarysensor_ != nullptr)
      this->motor_mainbrush_overcurrent_binarysensor_->publish_state(0);
    if (this->motor_rightwheel_overcurrent_binarysensor_ != nullptr)
      this->motor_rightwheel_overcurrent_binarysensor_->publish_state(0);
    if (this->motor_leftwheel_overcurrent_binarysensor_ != nullptr)
      this->motor_leftwheel_overcurrent_binarysensor_->publish_state(0);
    if (this->powerbutton_binarysensor_ != nullptr)
      this->powerbutton_binarysensor_->publish_state(0);
    if (this->spotbutton_binarysensor_ != nullptr)
      this->spotbutton_binarysensor_->publish_state(0);
    if (this->cleanbutton_binarysensor_ != nullptr)
      this->cleanbutton_binarysensor_->publish_state(0);

    return;
  }

  this->status_clear_warning();

  if (this->wall_binarysensor_ != nullptr)
    this->wall_binarysensor_->publish_state(response.reg.wall);
  if (this->bump_left_binarysensor_ != nullptr)
    this->bump_left_binarysensor_->publish_state(response.reg.bumps_wheeldrops & ROOMBA_MASK_BUMP_LEFT);
  if (this->bump_right_binarysensor_ != nullptr)
    this->bump_right_binarysensor_->publish_state(response.reg.bumps_wheeldrops & ROOMBA_MASK_BUMP_RIGHT);
  if (this->wheeldrop_left_binarysensor_ != nullptr)
    this->wheeldrop_left_binarysensor_->publish_state(response.reg.bumps_wheeldrops & ROOMBA_MASK_WHEELDROP_LEFT);
  if (this->wheeldrop_right_binarysensor_ != nullptr)
    this->wheeldrop_right_binarysensor_->publish_state(response.reg.bumps_wheeldrops & ROOMBA_MASK_WHEELDROP_RIGHT);
  if (this->wheeldrop_caster_binarysensor_ != nullptr)
    this->wheeldrop_caster_binarysensor_->publish_state(response.reg.bumps_wheeldrops & ROOMBA_MASK_WHEELDROP_CASTER);
  if (this->clift_left_binarysensor_ != nullptr)
    this->clift_left_binarysensor_->publish_state(response.reg.cliff_left);
  if (this->clift_frontleft_binarysensor_ != nullptr)
    this->clift_frontleft_binarysensor_->publish_state(response.reg.cliff_frontleft);
  if (this->clift_frontright_binarysensor_ != nullptr)
    this->clift_frontright_binarysensor_->publish_state(response.reg.cliff_frontright);
  if (this->clift_right_binarysensor_ != nullptr)
    this->clift_right_binarysensor_->publish_state(response.reg.cliff_right);
  if (this->virtualwall_binarysensor_ != nullptr)
    this->virtualwall_binarysensor_->publish_state(response.reg.virtual_wall);
  if (this->motor_sidebrush_overcurrent_binarysensor_ != nullptr)
    this->motor_sidebrush_overcurrent_binarysensor_->publish_state(response.reg.motor_overcurrents &
                                                                   ROOMBA_MASK_SIDE_BRUSH);
  if (this->motor_vacuum_overcurrent_binarysensor_ != nullptr)
    this->motor_vacuum_overcurrent_binarysensor_->publish_state(response.reg.motor_overcurrents & ROOMBA_MASK_VACUUM);
  if (this->motor_mainbrush_overcurrent_binarysensor_ != nullptr)
    this->motor_mainbrush_overcurrent_binarysensor_->publish_state(response.reg.motor_overcurrents &
                                                                   ROOMBA_MASK_MAIN_BRUSH);
  if (this->motor_rightwheel_overcurrent_binarysensor_ != nullptr)
    this->motor_rightwheel_overcurrent_binarysensor_->publish_state(response.reg.motor_overcurrents &
                                                                    ROOMBA_MASK_RIGHT_WHEEL);
  if (this->motor_leftwheel_overcurrent_binarysensor_ != nullptr)
    this->motor_leftwheel_overcurrent_binarysensor_->publish_state(response.reg.motor_overcurrents &
                                                                   ROOMBA_MASK_LEFT_WHEEL);
  if (this->powerbutton_binarysensor_ != nullptr)
    this->powerbutton_binarysensor_->publish_state(response.reg.buttons & ROOMBA_MASK_BUTTON_DOCK);
  if (this->spotbutton_binarysensor_ != nullptr)
    this->spotbutton_binarysensor_->publish_state(response.reg.buttons & ROOMBA_MASK_BUTTON_SPOT);
  if (this->cleanbutton_binarysensor_ != nullptr)
    this->cleanbutton_binarysensor_->publish_state(response.reg.buttons & ROOMBA_MASK_BUTTON_CLEAN);
}

}  // namespace roomba
}  // namespace esphome
