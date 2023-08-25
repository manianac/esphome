#pragma once

#include "esphome/core/component.h"
#include "esphome/components/i2c/i2c.h"
#include "esphome/components/time/real_time_clock.h"

namespace esphome {
namespace mcp7940 {

class MCP7940Component : public time::RealTimeClock, public i2c::I2CDevice {
 public:
  void setup() override;
  void update() override;
  void dump_config() override;
  float get_setup_priority() const override;
  void read_time();
  void write_time();
  void set_external_osc(const bool val) { this->external_osc_ = val; }
  void set_oscillator_trim(const int8_t val) { this->osc_trim_ = val; }
  void set_course_trim(const bool val) { this->course_osc_trim_ = val; }

 protected:
  bool read_rtc_();
  bool write_rtc_();
  bool halt_rtc_();
  union MCP7940Reg {
    struct {
      // 0x00 RTCSEC
      uint8_t second : 4;
      uint8_t second_10 : 3;
      bool start_osc : 1;
      // 0x01 RTCMIN
      uint8_t minute : 4;
      uint8_t minute_10 : 3;
      uint8_t unused_1 : 1;
      // 0x02 RTCHOUR
      uint8_t hour : 4;
      uint8_t hour_10 : 2;
      uint8_t hour_12_24 : 1;
      uint8_t unused_2 : 1;
      // 0x03 RTCWKDAY
      uint8_t weekday : 3;
      uint8_t unused_3 : 2;
      uint8_t osc_run : 1;
      uint8_t unused_3_1 : 2;
      // 0x04 RTCDATE
      uint8_t day : 4;
      uint8_t day_10 : 2;
      uint8_t unused_4 : 2;
      // 0x05 RTCMTH
      uint8_t month : 4;
      uint8_t month_10 : 1;
      uint8_t leap_year : 1;
      uint8_t unused_5 : 2;
      // 0x06 RTCYEAR
      uint8_t year : 4;
      uint8_t year_10 : 4;
      // 0x07 CONTROL
      uint8_t square_wave_freq : 2;
      bool course_trim_enable : 1;
      bool ext_osc_en : 1;
      bool alarm_0_en : 1;
      bool alarm_1_en : 1;
      bool squarewave_output_en : 1;
      bool out : 1;
      // 0x08 OSCTRIM
      int8_t trim_value; //Shifted right by 1 (so 1 == 2 clock cycles)
    } reg;
    mutable uint8_t raw[sizeof(reg)];
  } mcp7940_;
  bool external_osc_;
  bool course_osc_trim_;
  int16_t osc_trim_;
};

template<typename... Ts> class WriteAction : public Action<Ts...>, public Parented<MCP7940Component> {
 public:
  void play(Ts... x) override { this->parent_->write_time(); }
};

template<typename... Ts> class ReadAction : public Action<Ts...>, public Parented<MCP7940Component> {
 public:
  void play(Ts... x) override { this->parent_->read_time(); }
};
}  // namespace mcp7940
}  // namespace esphome
