#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"

namespace esphome {
namespace roomba {

class RoombaListener {
 public:
  union sensor_packet0 {
    struct __attribute__((__packed__)) {
      uint8_t bumps_wheeldrops;
      uint8_t wall;
      uint8_t cliff_left;
      uint8_t cliff_frontleft;
      uint8_t cliff_frontright;
      uint8_t cliff_right;
      uint8_t virtual_wall;
      uint8_t motor_overcurrents;
      uint8_t dirtdetector_left;
      uint8_t dirtdetector_right;
      uint8_t remote_opcode;
      uint8_t buttons;
      int16_t distance;
      int16_t angle;
      uint8_t charging_state;
      uint16_t voltage;
      int16_t current;
      int8_t temperature;
      uint16_t charge;
      uint16_t capacity;
    } reg;
    mutable uint8_t raw[sizeof(reg)];
  };
  union sensor_packet6 {
    struct __attribute__((__packed__)) {
      uint8_t bumps_wheeldrops;
      uint8_t wall;
      uint8_t cliff_left;
      uint8_t cliff_frontleft;
      uint8_t cliff_frontright;
      uint8_t cliff_right;
      uint8_t virtual_wall;
      uint8_t motor_overcurrents;
      uint8_t dirtdetector_left;
      uint8_t dirtdetector_right;
      uint8_t remote_opcode;
      uint8_t buttons;
      int16_t distance;
      int16_t angle;
      uint8_t charging_state;
      uint16_t voltage;
      int16_t current;
      int8_t temperature;
      uint16_t charge;
      uint16_t capacity;
      uint16_t wall_signal;
      uint16_t cliff_left_signal;
      uint16_t cliff_leftfront_signal;
      uint16_t cliff_rightfront_signal;
      uint16_t cliff_right_signal;
      uint8_t unk_1;
      uint16_t unk_2;
      uint8_t charger_available;
      uint8_t openinterface_mode;
      uint8_t song_number;
      uint8_t song_playing;
      uint8_t oi_stream_packets;
      int16_t velocity;
      int16_t radius;
      int16_t velocity_right;
      int16_t velocity_left;
    } reg;
    mutable uint8_t raw[sizeof(reg)];
  };

  virtual void onNewSensorData6(const sensor_packet6 &response) {}
  virtual void onNoSensorData() {}
};

class RoombaComponent : public Component, public uart::UARTDevice {
 public:
  float get_setup_priority() const override { return setup_priority::DATA; }

  void dump_config() override;
  void setup() override { this->write(128); }
  void loop() override;

  void register_listener(RoombaListener *listener) { this->listeners_.push_back(listener); }

 protected:
  typedef enum {
    SEND_STARTOI,
    SEND_SENSOR_REQUEST,
    READ_SENSOR_DATA,
    PROCESS_DATA,
  } loopState;
  typedef enum {
    Sensors7to26 = 0,
    Sensors7to16 = 1,
    Sensors17to20 = 2,
    Sensors21to26 = 3,
    Sensors27to34 = 4,
    Sensors35to42 = 5,
    Sensors7to42 = 6,
    SensorBumpsAndWheelDrops = 7,
    SensorWall = 8,
    SensorCliffLeft = 9,
    SensorCliffFrontLeft = 10,
    SensorCliffFrontRight = 11,
    SensorCliffRight = 12,
    SensorVirtualWall = 13,
    SensorOvercurrents = 14,
    //	SensorUnused1                  = 15,
    //	SensorUnused2                  = 16,
    SensorIRByte = 17,
    SensorButtons = 18,
    SensorDistance = 19,
    SensorAngle = 20,
    SensorChargingState = 21,
    SensorVoltage = 22,
    SensorCurrent = 23,
    SensorBatteryTemperature = 24,
    SensorBatteryCharge = 25,
    SensorBatteryCapacity = 26,
    SensorWallSignal = 27,
    SensoCliffLeftSignal = 28,
    SensoCliffFrontLeftSignal = 29,
    SensoCliffFrontRightSignal = 30,
    SensoCliffRightSignal = 31,
    SensorUserDigitalInputs = 32,
    SensorUserAnalogInput = 33,
    SensorChargingSourcesAvailable = 34,
    SensorOIMode = 35,
    SensorSongNumber = 36,
    SensorSongPlaying = 37,
    SensorNumberOfStreamPackets = 38,
    SensorVelocity = 39,
    SensorRadius = 40,
    SensorRightVelocity = 41,
    SensorLeftVelocity = 42,
  } Sensor;
  bool readSensorData(uint8_t *destBuffer, const size_t destBufferLength);

  std::vector<RoombaListener *> listeners_{};
};

}  // namespace roomba
}  // namespace esphome
