#pragma once

#include "esphome/components/sensor/sensor.h"
#include "esphome/components/voltage_sampler/voltage_sampler.h"
#include "esphome/core/component.h"
#include "esphome/core/hal.h"

#include "../ads79xx.h"

namespace esphome {
namespace ads79xx {

class ADS79XXSensor : public PollingComponent,
                      public Parented<ADS79XX>,
                      public sensor::Sensor,
                      public voltage_sampler::VoltageSampler {
 public:
  ADS79XXSensor() = default;

  void update() override;
  void dump_config() override;
  float get_setup_priority() const override;
  float sample() override;

  uint8_t get_channel() const { return this->channel_; }
  void set_channel(const uint8_t channel) { this->channel_ = channel; }
  
  float get_vref_voltage() const { return (this->double_vref_ ? 5.000 : 2.500); }

  void set_double_vref(const bool double_vref) { this->double_vref_ = double_vref; }
  bool get_double_vref() const { return this->double_vref_; }

 protected:
  uint8_t channel_;
  bool double_vref_;
};
}  // namespace ads79xx
}  // namespace esphome
