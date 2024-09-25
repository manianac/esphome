#include "RoombaTextSensor.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace esphome {
namespace roomba {

static const char *const TAG = "Roomba.text_sensor";

void RoombaTextSensor::dump_config() {
  ESP_LOGCONFIG(TAG, "Roomba TextSensor:");
  LOG_TEXT_SENSOR("  ", "Charging State", this->chg_state_text_sensor_);
  LOG_TEXT_SENSOR("  ", "Remote Opcode", this->chg_remote_opcode_sensor_);
}

void RoombaTextSensor::onNoSensorData() {
  if (this->chg_state_text_sensor_ != nullptr)
    this->chg_state_text_sensor_->publish_state("Unknown");

  if (this->chg_source_text_sensor_ != nullptr)
    this->chg_source_text_sensor_->publish_state("Unknown");

  if (this->chg_remote_opcode_sensor_ != nullptr)
    this->chg_remote_opcode_sensor_->publish_state("Unknown");
}

void RoombaTextSensor::onNewSensorData6(const sensor_packet6 &response) {
  ESP_LOGV(TAG, "Processing RoombaTextSensor::onNewData");
  size_t bufferLength = 1;
  std::string strSensorState = "Unknown";
  std::string strChargerAvailable = "Unknown";
  switch (response.reg.charging_state) {
    case ChargeStateNotCharging:
      strSensorState = "Not Charging";
      break;
    case ChargeStateReconditioningCharging:
      strSensorState = "Reconditioning Charging";
      break;
    case ChargeStateFullChanrging:
      strSensorState = "Full Charging";
      break;
    case ChargeStateTrickleCharging:
      strSensorState = "Trickle Charging";
      break;
    case ChargeStateWaiting:
      strSensorState = "Waiting";
      break;
    case ChargeStateFault:
      strSensorState = "Charge Fault";
      break;
  }

  switch (response.reg.charger_available) {
    case ChargeSourceNotCharging:
      strChargerAvailable = "Not Charging";
      break;
    case ChargeSourceInternal:
      strChargerAvailable = "Internal";
      break;
    case ChargeSourceHomeBase:
      strChargerAvailable = "Home Base";
      break;
  }

  if (this->chg_state_text_sensor_ != nullptr) {
    if (s_ChargeState_ != strSensorState) {
      s_ChargeState_ = strSensorState;
      this->chg_state_text_sensor_->publish_state(strSensorState);
    }
  }

  if (this->chg_source_text_sensor_ != nullptr) {
    if (s_ChargeSource_ != strChargerAvailable) {
      s_ChargeSource_ = strChargerAvailable;
      this->chg_source_text_sensor_->publish_state(strChargerAvailable);
    }
  }

  if (this->chg_remote_opcode_sensor_ != nullptr) {
    const std::string newRemoteOpcode = std::to_string(response.reg.remote_opcode);
    if (s_RemoteOpcode_ != newRemoteOpcode) {
      s_RemoteOpcode_ = newRemoteOpcode;
      this->chg_remote_opcode_sensor_->publish_state(newRemoteOpcode);
    }
  }
}

}  // namespace roomba
}  // namespace esphome
