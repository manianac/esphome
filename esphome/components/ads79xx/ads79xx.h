#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/components/spi/spi.h"

namespace esphome {
namespace ads79xx {

class ADS79XX : public Component,
                public spi::SPIDevice<spi::BIT_ORDER_MSB_FIRST, spi::CLOCK_POLARITY_LOW, spi::CLOCK_PHASE_LEADING,
                                      spi::DATA_RATE_10MHZ> {
  public:
    ADS79XX() = default;

    void set_reference_voltage(float reference_voltage) { this->reference_voltage_ = reference_voltage; }

    void setup() override;
    void loop() override;
    float get_loop_priority() const override { return 9.0f; }   // Just after WIFI
    void dump_config() override;
    float get_setup_priority() const override;
    float read_data(uint8_t channel);
  protected:
    float reference_voltage_;
    
    uint16_t manual_read(const uint8_t channel);

    uint16_t adc_counts_[16];
    uint16_t adc_read_;
};

}  // namespace ads79xx
}  // namespace esphome
