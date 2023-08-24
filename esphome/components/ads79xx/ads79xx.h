#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/components/spi/spi.h"

namespace esphome {
namespace ads79xx {

enum ADS79XXResolution {
  ADS795X_12_BITS = 12,
  ADS795X_10_BITS = 10,
  ADS79XX_8_BITS = 8,
};

class ADS79XXSensor;
class ADS79XX : public Component,
                public spi::SPIDevice<spi::BIT_ORDER_MSB_FIRST, spi::CLOCK_POLARITY_LOW, spi::CLOCK_PHASE_LEADING,
                                      spi::DATA_RATE_10MHZ> {
  public:
    ADS79XX() = default;

    void setup() override;
    void dump_config() override;
    float get_setup_priority() const override { return setup_priority::DATA; }
    float read_data(const ADS79XXSensor* sensor);

    void set_resolution(ADS79XXResolution resolution) { this->resolution_ = resolution; }
    uint8_t get_resolution() const { return resolution_; }
    float get_resolution_divider() const;
  protected:
    uint16_t manual_read_singleshot(const ADS79XXSensor* sensor);

    ADS79XXResolution resolution_;
};

}  // namespace ads79xx
}  // namespace esphome
