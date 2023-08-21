#include "ads79xx_sensor.h"

#include "esphome/core/log.h"

namespace esphome {
namespace ads79xx {

static const char *const TAG = "ads79xx.sensor";

ADS79XXSensor::ADS79XXSensor(uint8_t channel) : channel_(channel) {}

float ADS79XXSensor::get_setup_priority() const { return setup_priority::DATA; }

void ADS79XXSensor::dump_config() {
  LOG_SENSOR("", "ADS79XX Sensor", this);
  ESP_LOGCONFIG(TAG, "  Pin: %u", this->channel_);
  LOG_UPDATE_INTERVAL(this);
}

float ADS79XXSensor::sample() { return this->parent_->read_data(this->channel_); }
void ADS79XXSensor::update() { this->publish_state(this->sample()); }

}  // namespace ads79xx
}  // namespace esphome
