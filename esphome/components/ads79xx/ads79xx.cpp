#include "ads79xx.h"
#include "esphome/core/log.h"

namespace esphome {
namespace ads79xx {

static constexpr char TAG[] = "ads79xx";

static constexpr uint8_t MODECONTROL_MANUAL        = 0b0001;
static constexpr uint8_t MODECONTROL_AUTO1         = 0b0010;
static constexpr uint8_t MODECONTROL_AUTO2         = 0b0011;
static constexpr uint8_t MODECONTROL_GPIO          = 0b0100;
static constexpr uint8_t MODECONTROL_AUTO1PROGRAM  = 0b1000;
static constexpr uint8_t MODECONTROL_AUTO2PROGRAM  = 0b1001;

// Datasheet: https://www.ti.com/lit/ds/symlink/ads7953.pdf

void ADS79XX::setup() {
  ESP_LOGCONFIG(TAG, "Setting up ads79xx pins");
  this->spi_setup();

  this->enable();
  this->write_byte16( (MODECONTROL_MANUAL << 12) |
                      (1 << 11) |          // Enable setting of configuration bits
                      (1 << 6) );          // Enable 2xVref
  this->disable();

  this->enable();
  this->write_byte16( (MODECONTROL_GPIO << 12) | 1);  // Configure GPIO0 as output (Valid for both TSOP and QFN devices)
  this->disable();
}

void ADS79XX::dump_config() {
  ESP_LOGCONFIG(TAG,   "ads79xx:");
  LOG_PIN(             "  CS Pin: ", this->cs_);
  ESP_LOGCONFIG(TAG,   "  Reference Voltage: %.2fV", this->reference_voltage_);
}

// Prepare a manual read message with configured channel.  Re-use other configuration bits from previous frame in setup().
// Returns entire frame from A/D with channel and 12-bit adc count.
uint16_t ADS79XX::manual_read_singleshot(const uint8_t channel)
{
  uint16_t config = (MODECONTROL_MANUAL << 12) | (channel << 7); // Manual read of configured channel

  this->enable();
  uint8_t adc_primary_byte = this->transfer_byte(config >> 8);
  uint8_t adc_secondary_byte = this->transfer_byte(config & 0xFF);
  this->disable();

  return (adc_primary_byte << 8 | adc_secondary_byte);
}

// Attempt a manual read of the A/D channel, returning the voltage based off of the configured reference voltage.
// Returns voltage readout of channel, or NAN if the read failed.
float ADS79XX::read_data(uint8_t channel)
{
  uint16_t adc_counts = 0;
  uint8_t channel_id = 0xFF; // Start with an invalid channel to always force the first read

  // The datasheet specifies three SPI transfers to return an A/D acquisition.  But in the event of multiple reads of the same channel, 
  // return the earliest available result (from a previous call).
  for (int i=0; i<3 && (channel_id != channel); i++)
  {
    adc_counts = manual_read_singleshot(channel);
    channel_id = (adc_counts >> 12);
    ESP_LOGV(TAG, "A/D Manual Read: Attempt: %d, Req: %d, Recv Chan: %d, Recv: 0x%04X", i+1, channel, channel_id, adc_counts);
  }
  
  if (channel_id != channel)
  {
    this->mark_failed();
    ESP_LOGE(TAG, "Failed to read channel %d, received channel %d: 0x%04X", channel, channel_id, adc_counts);
    return NAN;
  }

  uint16_t digital_value = adc_counts & 0x0FFF; // Strip channel ID

  return (digital_value / 4096.000) * this->reference_voltage_;
}

}  // namespace ads79xx
}  // namespace esphome
