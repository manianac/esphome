#pragma once

#include "../RoombaComponent.h"
#include "esphome/components/text_sensor/text_sensor.h"

namespace esphome {
namespace roomba {

class RoombaTextSensor : public RoombaListener, public Component {
 public:
  void dump_config() override;
  void set_charging_state_text_sensor(text_sensor::TextSensor *tsensor) { this->chg_state_text_sensor_ = tsensor; };
  void set_charging_source_text_sensor(text_sensor::TextSensor *tsensor) { this->chg_source_text_sensor_ = tsensor; };
  void set_remote_opcode_text_sensor(text_sensor::TextSensor *tsensor) { this->chg_remote_opcode_sensor_ = tsensor; };
  void onNewSensorData6(const sensor_packet6 &response) override;
  void onNoSensorData() override;

 protected:
  /// \enum ChargeState
  /// Values for sensor packet ID 21
  typedef enum {
    ChargeStateNotCharging = 0,
    ChargeStateReconditioningCharging = 1,
    ChargeStateFullChanrging = 2,
    ChargeStateTrickleCharging = 3,
    ChargeStateWaiting = 4,
    ChargeStateFault = 5,
  } ChargeState;

  typedef enum {
    ChargeSourceNotCharging = 0,
    ChargeSourceInternal = 1,
    ChargeSourceHomeBase = 2,
  } ChargeSource;

  text_sensor::TextSensor *chg_state_text_sensor_{nullptr};
  std::string s_ChargeState_;
  text_sensor::TextSensor *chg_source_text_sensor_{nullptr};
  std::string s_ChargeSource_;
  text_sensor::TextSensor *chg_remote_opcode_sensor_{nullptr};
  std::string s_RemoteOpcode_;
};

}  // namespace roomba
}  // namespace esphome
