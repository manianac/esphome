import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    CONF_ID,
    CONF_BATTERY_VOLTAGE,
    STATE_CLASS_MEASUREMENT,
    STATE_CLASS_NONE,
    DEVICE_CLASS_VOLTAGE,
    DEVICE_CLASS_CURRENT,
    DEVICE_CLASS_TEMPERATURE,
    UNIT_CELSIUS,
    UNIT_VOLT,
    UNIT_AMPERE,
    ICON_BATTERY,
    ICON_THERMOMETER,
)
from .. import roomba_ns, RoombaComponent, CONF_ROOMBA_ID

DEPENDENCIES = ["roomba"]

RoombaSensor = roomba_ns.class_("RoombaSensor", sensor.Sensor, cg.Component)

CONF_BATTERY_CURRENT = "battery_current"
CONF_BATTERY_CHARGE = "battery_charge"
CONF_BATTERY_CAPACITY = "battery_capacity"
CONF_BATTERY_TEMPERATURE = "battery_temperature"
CONF_DIRTSENSOR_LEFT = "dirtsensor_left"
CONF_DIRTSENSOR_RIGHT = "dirtsensor_right"
CONF_LEFT_MOTOR_CURRENT = "left_motor_current"
CONF_RIGHT_MOTOR_CURRENT = "right_motor_current"
CONF_MAIN_BRUSH_CURRENT = "main_brush_current"
CONF_SIDE_BRUSH_CURRENT = "side_brush_current"

UNIT_AMPERE_HOUR = "Ah"

CONFIG_SCHEMA = cv.All(
    cv.COMPONENT_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(RoombaSensor),
            cv.GenerateID(CONF_ROOMBA_ID): cv.use_id(RoombaComponent),
            cv.Optional(CONF_BATTERY_VOLTAGE): sensor.sensor_schema(
                unit_of_measurement=UNIT_VOLT,
                icon=ICON_BATTERY,
                accuracy_decimals=3,
                device_class=DEVICE_CLASS_VOLTAGE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_BATTERY_CURRENT): sensor.sensor_schema(
                unit_of_measurement=UNIT_AMPERE,
                icon=ICON_BATTERY,
                device_class=DEVICE_CLASS_CURRENT,
                accuracy_decimals=3,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_BATTERY_CHARGE): sensor.sensor_schema(
                unit_of_measurement=UNIT_AMPERE_HOUR,
                icon=ICON_BATTERY,
                accuracy_decimals=3,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_BATTERY_CAPACITY): sensor.sensor_schema(
                unit_of_measurement=UNIT_AMPERE_HOUR,
                icon=ICON_BATTERY,
                accuracy_decimals=3,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_BATTERY_TEMPERATURE): sensor.sensor_schema(
                unit_of_measurement=UNIT_CELSIUS,
                icon=ICON_THERMOMETER,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_DIRTSENSOR_LEFT): sensor.sensor_schema(
                accuracy_decimals=0,
                state_class=STATE_CLASS_NONE,
            ),
            cv.Optional(CONF_DIRTSENSOR_RIGHT): sensor.sensor_schema(
                accuracy_decimals=0,
                state_class=STATE_CLASS_NONE,
            ),
        }
    )
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    if CONF_BATTERY_VOLTAGE in config:
        sens = await sensor.new_sensor(config[CONF_BATTERY_VOLTAGE])
        cg.add(var.set_batteryvoltage_sensor(sens))

    if CONF_BATTERY_CURRENT in config:
        sens = await sensor.new_sensor(config[CONF_BATTERY_CURRENT])
        cg.add(var.set_batterycurrent_sensor(sens))

    if CONF_BATTERY_CHARGE in config:
        sens = await sensor.new_sensor(config[CONF_BATTERY_CHARGE])
        cg.add(var.set_batterycharge_sensor(sens))

    if CONF_BATTERY_CAPACITY in config:
        sens = await sensor.new_sensor(config[CONF_BATTERY_CAPACITY])
        cg.add(var.set_batterycapacity_sensor(sens))

    if CONF_BATTERY_TEMPERATURE in config:
        sens = await sensor.new_sensor(config[CONF_BATTERY_TEMPERATURE])
        cg.add(var.set_batterytemperature_sensor(sens))

    if CONF_DIRTSENSOR_LEFT in config:
        sens = await sensor.new_sensor(config[CONF_DIRTSENSOR_LEFT])
        cg.add(var.set_dirtsensor_left_sensor(sens))

    if CONF_DIRTSENSOR_RIGHT in config:
        sens = await sensor.new_sensor(config[CONF_DIRTSENSOR_RIGHT])
        cg.add(var.set_dirtsensor_right_sensor(sens))

    roomba = await cg.get_variable(config[CONF_ROOMBA_ID])
    cg.add(roomba.register_listener(var))
