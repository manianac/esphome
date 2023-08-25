#include "mcp7940.h"
#include "esphome/core/log.h"

// Datasheet:
// - https://ww1.microchip.com/downloads/en/DeviceDoc/MCP7940M-Low-Cost%20I2C-RTCC-with-SRAM-20002292C.pdf

namespace esphome {
namespace mcp7940 {

static const char *const TAG = "mcp7940";

void MCP7940Component::setup() {
  ESP_LOGCONFIG(TAG, "Setting up MCP7940...");
  if (!this->read_rtc_()) {
    this->mark_failed();
  }
}

void MCP7940Component::update() { this->read_time(); }

void MCP7940Component::dump_config() {
  ESP_LOGCONFIG(TAG, "MCP7940:");
  LOG_I2C_DEVICE(this);
  if (this->is_failed()) {
    ESP_LOGE(TAG, "Communication with MCP7940 failed!");
  }
  ESP_LOGCONFIG(TAG, "  Timezone: '%s'", this->timezone_.c_str());
}

float MCP7940Component::get_setup_priority() const { return setup_priority::DATA; }

void MCP7940Component::read_time() {
  if (!this->read_rtc_()) {
    return;
  }
  if (!mcp7940_.reg.osc_run) {
    ESP_LOGW(TAG, "RTC halted, not syncing to system clock.");
    return;
  }
  ESPTime rtc_time{.second = uint8_t(mcp7940_.reg.second + 10 * mcp7940_.reg.second_10),
                   .minute = uint8_t(mcp7940_.reg.minute + 10u * mcp7940_.reg.minute_10),
                   .hour = uint8_t(mcp7940_.reg.hour + 10u * mcp7940_.reg.hour_10),
                   .day_of_week = uint8_t(mcp7940_.reg.weekday),
                   .day_of_month = uint8_t(mcp7940_.reg.day + 10u * mcp7940_.reg.day_10),
                   .day_of_year = 1,  // ignored by recalc_timestamp_utc(false)
                   .month = uint8_t(mcp7940_.reg.month + 10u * mcp7940_.reg.month_10),
                   .year = uint16_t(mcp7940_.reg.year + 10u * mcp7940_.reg.year_10 + 2000)};
  rtc_time.recalc_timestamp_utc(false);
  if (!rtc_time.is_valid()) {
    ESP_LOGE(TAG, "Invalid RTC time, not syncing to system clock.");
    return;
  }
  time::RealTimeClock::synchronize_epoch_(rtc_time.timestamp);
}

void MCP7940Component::write_time() {
  this->halt_rtc_();
  auto now = time::RealTimeClock::utcnow();
  if (!now.is_valid()) {
    ESP_LOGE(TAG, "Invalid system time, not syncing to RTC.");
    return;
  }
  
  this->mcp7940_.reg.year = (now.year - 2000) % 10;
  this->mcp7940_.reg.year_10 = (now.year - 2000) / 10 % 10;
  this->mcp7940_.reg.month = now.month % 10;
  this->mcp7940_.reg.month_10 = now.month / 10;
  this->mcp7940_.reg.day = now.day_of_month % 10;
  this->mcp7940_.reg.day_10 = now.day_of_month / 10;
  this->mcp7940_.reg.weekday = now.day_of_week;
  this->mcp7940_.reg.hour = now.hour % 10;
  this->mcp7940_.reg.hour_10 = now.hour / 10;
  this->mcp7940_.reg.minute = now.minute % 10;
  this->mcp7940_.reg.minute_10 = now.minute / 10;
  this->mcp7940_.reg.second = now.second % 10;
  this->mcp7940_.reg.second_10 = now.second / 10;
  this->mcp7940_.reg.start_osc = true;
  this->mcp7940_.reg.ext_osc_en = this->external_osc_;
  this->mcp7940_.reg.hour_12_24 = 0;

  this->write_rtc_();
}

bool MCP7940Component::read_rtc_() {
  if (!this->read_bytes(0, this->mcp7940_.raw, sizeof(this->mcp7940_.raw))) {
    ESP_LOGE(TAG, "Can't read I2C data.");
    return false;
  }
  ESP_LOGD(TAG, "Read  %0u%0u:%0u%0u:%0u%0u 20%0u%0u-%0u%0u-%0u%0u  ST:%s CRSTRIM:%s EXT_OSC:%s 12_24:%s OSC_RUN:%s SQWE:%s OUT:%s TRIM:%d", mcp7940_.reg.hour_10,
           mcp7940_.reg.hour, mcp7940_.reg.minute_10, mcp7940_.reg.minute, mcp7940_.reg.second_10, mcp7940_.reg.second,
           mcp7940_.reg.year_10, mcp7940_.reg.year, mcp7940_.reg.month_10, mcp7940_.reg.month, mcp7940_.reg.day_10,
           mcp7940_.reg.day, ONOFF(mcp7940_.reg.start_osc), ONOFF(mcp7940_.reg.course_trim_enable), ONOFF(mcp7940_.reg.ext_osc_en),
           ONOFF(mcp7940_.reg.hour_12_24), ONOFF(mcp7940_.reg.osc_run), ONOFF(mcp7940_.reg.squarewave_output_en), ONOFF(mcp7940_.reg.out), mcp7940_.reg.trim_value);

  return true;
}

bool MCP7940Component::write_rtc_() {
  if (!this->write_bytes(0, this->mcp7940_.raw, sizeof(this->mcp7940_.raw))) {
    ESP_LOGE(TAG, "Can't write I2C data.");
    return false;
  }
  ESP_LOGD(TAG, "Write  %0u%0u:%0u%0u:%0u%0u 20%0u%0u-%0u%0u-%0u%0u  ST:%s CRSTRIM:%s EXT_OSC:%s 12_24:%s OSC_RUN:%s SQWE:%s OUT:%s TRIM:%d", mcp7940_.reg.hour_10,
           mcp7940_.reg.hour, mcp7940_.reg.minute_10, mcp7940_.reg.minute, mcp7940_.reg.second_10, mcp7940_.reg.second,
           mcp7940_.reg.year_10, mcp7940_.reg.year, mcp7940_.reg.month_10, mcp7940_.reg.month, mcp7940_.reg.day_10,
           mcp7940_.reg.day, ONOFF(mcp7940_.reg.start_osc), ONOFF(mcp7940_.reg.course_trim_enable), ONOFF(mcp7940_.reg.ext_osc_en),
           ONOFF(mcp7940_.reg.hour_12_24), ONOFF(mcp7940_.reg.osc_run), ONOFF(mcp7940_.reg.squarewave_output_en), ONOFF(mcp7940_.reg.out), mcp7940_.reg.trim_value);
  return true;
}
bool MCP7940Component::halt_rtc_() {
  while (this->mcp7940_.reg.osc_run)
  {
    this->mcp7940_.reg.start_osc = false;
    if (!this->write_bytes(0, this->mcp7940_.raw, sizeof(this->mcp7940_.raw))) {
      ESP_LOGE(TAG, "Can't write I2C data.");
      return false;
    }
    else delay_microseconds_safe(4); // Wait at least 3.5us for the clock to be valid (start/stop)
    ESP_LOGD(TAG, "Halt  ST:%s CRSTRIM:%s OSC_RUN:%s EXT_OSC:%s SQWE:%s OUT:%s", 
            ONOFF(mcp7940_.reg.start_osc), ONOFF(mcp7940_.reg.course_trim_enable), ONOFF(mcp7940_.reg.osc_run), ONOFF(mcp7940_.reg.ext_osc_en),
            ONOFF(mcp7940_.reg.squarewave_output_en), ONOFF(mcp7940_.reg.out));
    this->read_rtc_();
  }
  return true;
}
}  // namespace mcp7940
}  // namespace esphome
