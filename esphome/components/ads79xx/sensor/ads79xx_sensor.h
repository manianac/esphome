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
  ADS79XXSensor(uint8_t channel);

  void update() override;
  void dump_config() override;
  float get_setup_priority() const override;
  float sample() override;

 protected:
  uint8_t channel_;
};
}  // namespace ads79xx
}  // namespace esphome
