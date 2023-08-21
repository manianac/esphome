#include "ads79xx.h"
#include "esphome/core/log.h"

namespace esphome {
namespace ads79xx {

static const char *const TAG = "ads79xx";

// Datasheet: https://www.ti.com/lit/ds/symlink/ads7953.pdf

float ADS79XX::get_setup_priority() const { return setup_priority::HARDWARE; }

void ADS79XX::setup() {
  ESP_LOGCONFIG(TAG, "Setting up ads79xx pins");
  this->spi_setup();
}

void ADS79XX::dump_config() {
  ESP_LOGCONFIG(TAG, "ads79xx:");
  LOG_PIN("  CS Pin: ", this->cs_);
  ESP_LOGCONFIG(TAG, "  Reference Voltage: %.2fV", this->reference_voltage_);
}

float ADS79XX::read_data(uint8_t channel) {
  this->enable();
  uint16_t config = 0b0001100001000000 | (channel << 7);
  this->write_byte16(config); // Send initial read command
  this->disable();
  delayMicroseconds(5); // Datasheet says only 1125ns are needed, but leave some wiggle room
  this->enable();
  this->write_byte16(0b0100000000000001); // Set up GPIO as output
  this->disable();
  delayMicroseconds(5); // Datasheet says only 1125ns are needed, but leave some wiggle room
  this->enable();
  config = 0b0001100001000001 | (channel << 7);
  uint8_t adc_primary_byte = this->transfer_byte(config >> 8);
  uint8_t adc_secondary_byte = this->transfer_byte(config & 0xFF);
  this->disable();

  uint8_t channel_id = (adc_primary_byte >> 4);
  uint16_t digital_value = (adc_primary_byte << 8 | adc_secondary_byte) & 0x0FFF; // Strip channel ID

  ESP_LOGV(TAG, "Requested A/D Read of channel %d.  Received channel %d , Counts 0x%04X", channel, channel_id, digital_value);

  if (channel_id != channel)
  {
    this->mark_failed();
    return NAN;
  }

  return (digital_value / 4096.000) * this->reference_voltage_;
}

}  // namespace ads79xx
}  // namespace esphome
