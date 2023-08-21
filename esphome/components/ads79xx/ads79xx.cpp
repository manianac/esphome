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

  this->enable();
  this->write_byte16(0b0010110001000000); // Configure automatic mode
  this->disable();
  delayMicroseconds(3);
  this->enable();
  this->write_byte16(0b0100000000000001); // Configure GPIO0 as output
  this->disable();
  delayMicroseconds(3);
  this->enable();
  this->write_byte16(0b1000000000000000); // Configure auto-1 register
  this->disable();
  delayMicroseconds(3);
  this->enable();
  this->write_byte16(0b1111111111111111); // All channels
  this->disable();
}

void ADS79XX::dump_config() {
  ESP_LOGCONFIG(TAG, "ads79xx:");
  LOG_PIN("  CS Pin: ", this->cs_);
  ESP_LOGCONFIG(TAG, "  Reference Voltage: %.2fV", this->reference_voltage_);
}

uint16_t ADS79XX::manual_read(const uint8_t channel)
{
  ESP_LOGD(TAG, "Doing a manual read on channel %d", channel);
  this->enable();
  uint16_t config = 0b0001100001000000 | (channel << 7); // Manual read of configured channel, turn on GPIO0 for visualization
  this->write_byte16(config); // Send initial read command
  this->disable();
  delayMicroseconds(3); // Datasheet says only 1125ns are needed, but leave some wiggle room

  this->enable();
  this->write_byte16(0); // Waste a cycle for allow for acquisition
  this->disable();
  delayMicroseconds(3); // Datasheet says only 1125ns are needed, but leave some wiggle room

  this->enable();
  config = 0b0001100001000001 | (channel << 7); //Turn off GPIO as acquisition is completed
  uint8_t adc_primary_byte = this->transfer_byte(config >> 8);
  uint8_t adc_secondary_byte = this->transfer_byte(config & 0xFF);
  this->disable();

  return (adc_primary_byte << 8 | adc_secondary_byte);
}

void ADS79XX::loop()
{
  this->enable();
  uint16_t config = 0b0010000000000000; // Auto Read
  uint8_t adc_primary_byte = this->transfer_byte(config >> 8);
  uint8_t adc_secondary_byte = this->transfer_byte(config & 0xFF);
  this->disable();
  //delayMicroseconds(3); // Datasheet says only 1125ns are needed, but leave some wiggle room

  uint16_t adc_counts = (adc_primary_byte << 8 | adc_secondary_byte);

  uint8_t channel_id = (adc_counts >> 12);
  //uint16_t digital_value = adc_counts & 0x0FFF; // Strip channel ID

  this->adc_counts_[channel_id] = adc_counts;
  this->adc_read_ &= ~(1 << channel_id);
}

float ADS79XX::read_data(uint8_t channel)
{
  uint16_t adc_read = 0;
  if (this->adc_read_ & (1 << channel))
    adc_read = this->manual_read(channel);
  else
  {
    adc_read = this->adc_counts_[channel];
    this->adc_read_ |= (1 << channel);
  }

  uint8_t channel_id = (adc_read >> 12);
  uint16_t digital_value = adc_read & 0x0FFF; // Strip channel ID

  ESP_LOGV(TAG, "Requested A/D Read of channel %d.  Received channel %d , Counts 0x%04X", channel, channel_id, digital_value);

  if (channel_id != channel)
  {
    ESP_LOGE(TAG, "Failed to read channel %d, received %d", channel, channel_id);
    return NAN;
  }

  return (digital_value / 4096.000) * this->reference_voltage_;
}

}  // namespace ads79xx
}  // namespace esphome
