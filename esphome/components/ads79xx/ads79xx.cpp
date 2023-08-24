#include "ads79xx.h"
#include "sensor/ads79xx_sensor.h"
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

  ESP_LOGCONFIG(TAG, "Setting up ads79xx registers");
  this->enable();
  this->write_byte16( (MODECONTROL_GPIO << 12) |
                      ( 1 << 9 ));              // Reset GPIO settings to defaults
  this->disable();
}

void ADS79XX::dump_config() {
  ESP_LOGCONFIG(TAG,   "ads79xx:");
  LOG_PIN(             "  CS Pin: ", this->cs_);
  ESP_LOGCONFIG(TAG,   "  Resolution: %u", this->get_resolution());
}

// Prepare a manual read message with configured channel.  Re-use other configuration bits from previous frame in setup().
// Returns entire frame from A/D with channel and 12-bit adc count.
uint16_t ADS79XX::manual_read_singleshot(const ADS79XXSensor* sensor)
{
  uint16_t config = (MODECONTROL_MANUAL << 12) |         // Manual read of configured channel
                    ( 1 << 11 ) |                        // Enable setting of configuration functions
                    (sensor->get_channel() << 7) |       // Channel to request
                    (sensor->get_double_vref() << 6);    // Vref input range

  this->enable();
  uint8_t adc_primary_byte = this->transfer_byte(config >> 8);
  uint8_t adc_secondary_byte = this->transfer_byte(config & 0xFF);
  this->disable();

  return (adc_primary_byte << 8 | adc_secondary_byte);
}

float ADS79XX::get_resolution_divider() const
{
  switch (this->resolution_)
  {
    case ADS795X_12_BITS:
      return 4096.000;
    case ADS795X_10_BITS:
      return 2048.000;
    case ADS79XX_8_BITS:
      return 1024.000;
  }
  return NAN;
}

// Attempt a manual read of the A/D channel, returning the voltage based off of the configured reference voltage.
// Returns voltage readout of channel, or NAN if the read failed.
float ADS79XX::read_data(const ADS79XXSensor* sensor)
{
  uint16_t adc_counts = 0;
  uint8_t channel_id = 0xFF; // Start with an invalid channel to always force the first read

  // The datasheet specifies three SPI transfers to return an A/D acquisition.  But in the event of multiple reads of the same channel, 
  // return the earliest available result (from a previous call).

  for (int i=0; i<3 && (channel_id != sensor->get_channel()); i++)
  {
    adc_counts = manual_read_singleshot(sensor);
    channel_id = (adc_counts >> 12);
    ESP_LOGV(TAG, "A/D Manual Read: Attempt: %d, Req: %d, Recv Chan: %d, Recv: 0x%04X", i+1, sensor->get_channel(), channel_id, adc_counts);
  }

  if (channel_id != sensor->get_channel())
  {
    this->mark_failed();
    ESP_LOGE(TAG, "Failed to read channel %d, received channel %d: 0x%04X", sensor->get_channel(), channel_id, adc_counts);
    return NAN;
  }

  uint16_t digital_value = adc_counts & 0x0FFF; // Strip channel ID

  return (digital_value / this->get_resolution_divider()) * sensor->get_vref_voltage();
}

}  // namespace ads79xx
}  // namespace esphome
