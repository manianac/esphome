import esphome.codegen as cg
from esphome.components import binary_sensor
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID,
    DEVICE_CLASS_PROBLEM,
)

from .. import roomba_ns, RoombaComponent, CONF_ROOMBA_ID

AUTO_LOAD = ["binary_sensor"]
DEPENDENCIES = ["roomba"]

RoombaBinarySensor = roomba_ns.class_(
    "RoombaBinarySensor", binary_sensor.BinarySensor, cg.Component
)

CONF_LEFTBUMP_SENSOR = "left_bumpsensor"
CONF_RIGHTBUMP_SENSOR = "right_bumpsensor"
CONF_WALL_SENSOR = "wall_sensor"
CONF_LEFT_WHEELDROP_SENSOR = "left_wheeldrop"
CONF_RIGHT_WHEELDROP_SENSOR = "right_wheeldrop"
CONF_CASTER_WHEELDROP_SENSOR = "caster_wheeldrop"
CONF_CLIFF_LEFT_SENSOR = "cliff_left"
CONF_CLIFF_FRONTLEFT_SENSOR = "cliff_frontleft"
CONF_CLIFF_FRONTRIGHT_SENSOR = "cliff_frontright"
CONF_CLIFF_RIGHT_SENSOR = "cliff_right"
CONF_VIRTUALWALL_SENSOR = "virtualwall_sensor"
CONF_MOTOR_SIDEBRUSH_OVERCURRENT_SENSOR = "motor_sidebrush_overcurrent"
CONF_MOTOR_VACUUM_OVERCURRENT_SENSOR = "motor_vacuum_overcurrent"
CONF_MOTOR_MAINBRUSH_OVERCURRENT_SENSOR = "motor_mainbrush_overcurrent"
CONF_MOTOR_RIGHTWHEEL_OVERCURRENT_SENSOR = "motor_rightwheel_overcurrent"
CONF_MOTOR_LEFTWHEEL_OVERCURRENT_SENSOR = "motor_leftwheel_overcurrent"
CONF_POWER_BUTTON = "dock_button"
CONF_SPOT_BUTTON = "spot_button"
CONF_CLEAN_BUTTON = "clean_button"

CONFIG_SCHEMA = cv.All(
    cv.COMPONENT_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(RoombaBinarySensor),
            cv.GenerateID(CONF_ROOMBA_ID): cv.use_id(RoombaComponent),
            cv.Optional(CONF_WALL_SENSOR): binary_sensor.binary_sensor_schema(),
            cv.Optional(CONF_LEFTBUMP_SENSOR): binary_sensor.binary_sensor_schema(),
            cv.Optional(CONF_RIGHTBUMP_SENSOR): binary_sensor.binary_sensor_schema(),
            cv.Optional(CONF_LEFT_WHEELDROP_SENSOR): binary_sensor.binary_sensor_schema(
                device_class=DEVICE_CLASS_PROBLEM,
            ),
            cv.Optional(
                CONF_RIGHT_WHEELDROP_SENSOR
            ): binary_sensor.binary_sensor_schema(
                device_class=DEVICE_CLASS_PROBLEM,
            ),
            cv.Optional(
                CONF_CASTER_WHEELDROP_SENSOR
            ): binary_sensor.binary_sensor_schema(
                device_class=DEVICE_CLASS_PROBLEM,
            ),
            cv.Optional(CONF_CLIFF_LEFT_SENSOR): binary_sensor.binary_sensor_schema(),
            cv.Optional(
                CONF_CLIFF_FRONTLEFT_SENSOR
            ): binary_sensor.binary_sensor_schema(),
            cv.Optional(
                CONF_CLIFF_FRONTRIGHT_SENSOR
            ): binary_sensor.binary_sensor_schema(),
            cv.Optional(CONF_CLIFF_RIGHT_SENSOR): binary_sensor.binary_sensor_schema(),
            cv.Optional(CONF_VIRTUALWALL_SENSOR): binary_sensor.binary_sensor_schema(),
            cv.Optional(
                CONF_MOTOR_SIDEBRUSH_OVERCURRENT_SENSOR
            ): binary_sensor.binary_sensor_schema(
                device_class=DEVICE_CLASS_PROBLEM,
            ),
            cv.Optional(
                CONF_MOTOR_VACUUM_OVERCURRENT_SENSOR
            ): binary_sensor.binary_sensor_schema(
                device_class=DEVICE_CLASS_PROBLEM,
            ),
            cv.Optional(
                CONF_MOTOR_MAINBRUSH_OVERCURRENT_SENSOR
            ): binary_sensor.binary_sensor_schema(
                device_class=DEVICE_CLASS_PROBLEM,
            ),
            cv.Optional(
                CONF_MOTOR_RIGHTWHEEL_OVERCURRENT_SENSOR
            ): binary_sensor.binary_sensor_schema(
                device_class=DEVICE_CLASS_PROBLEM,
            ),
            cv.Optional(
                CONF_MOTOR_LEFTWHEEL_OVERCURRENT_SENSOR
            ): binary_sensor.binary_sensor_schema(
                device_class=DEVICE_CLASS_PROBLEM,
            ),
            cv.Optional(CONF_POWER_BUTTON): binary_sensor.binary_sensor_schema(),
            cv.Optional(CONF_SPOT_BUTTON): binary_sensor.binary_sensor_schema(),
            cv.Optional(CONF_CLEAN_BUTTON): binary_sensor.binary_sensor_schema(),
        }
    )
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    if CONF_WALL_SENSOR in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_WALL_SENSOR])
        cg.add(var.set_wall_sensor(sens))
    if CONF_LEFTBUMP_SENSOR in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_LEFTBUMP_SENSOR])
        cg.add(var.set_leftbump_sensor(sens))
    if CONF_RIGHTBUMP_SENSOR in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_RIGHTBUMP_SENSOR])
        cg.add(var.set_rightbump_sensor(sens))
    if CONF_LEFT_WHEELDROP_SENSOR in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_LEFT_WHEELDROP_SENSOR])
        cg.add(var.set_leftwheeldrop_sensor(sens))
    if CONF_RIGHT_WHEELDROP_SENSOR in config:
        sens = await binary_sensor.new_binary_sensor(
            config[CONF_RIGHT_WHEELDROP_SENSOR]
        )
        cg.add(var.set_rightwheeldrop_sensor(sens))
    if CONF_CASTER_WHEELDROP_SENSOR in config:
        sens = await binary_sensor.new_binary_sensor(
            config[CONF_CASTER_WHEELDROP_SENSOR]
        )
        cg.add(var.set_casterwheeldrop_sensor(sens))
    if CONF_CLIFF_LEFT_SENSOR in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_CLIFF_LEFT_SENSOR])
        cg.add(var.set_cliffleft_sensor(sens))
    if CONF_CLIFF_FRONTLEFT_SENSOR in config:
        sens = await binary_sensor.new_binary_sensor(
            config[CONF_CLIFF_FRONTLEFT_SENSOR]
        )
        cg.add(var.set_cliffleftfront_sensor(sens))
    if CONF_CLIFF_FRONTRIGHT_SENSOR in config:
        sens = await binary_sensor.new_binary_sensor(
            config[CONF_CLIFF_FRONTRIGHT_SENSOR]
        )
        cg.add(var.set_cliffrightfront_sensor(sens))
    if CONF_CLIFF_RIGHT_SENSOR in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_CLIFF_RIGHT_SENSOR])
        cg.add(var.set_cliffright_sensor(sens))
    if CONF_VIRTUALWALL_SENSOR in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_VIRTUALWALL_SENSOR])
        cg.add(var.set_virtualwall_sensor(sens))
    if CONF_MOTOR_SIDEBRUSH_OVERCURRENT_SENSOR in config:
        sens = await binary_sensor.new_binary_sensor(
            config[CONF_MOTOR_SIDEBRUSH_OVERCURRENT_SENSOR]
        )
        cg.add(var.set_sidebrush_overcurrent_sensor(sens))
    if CONF_MOTOR_VACUUM_OVERCURRENT_SENSOR in config:
        sens = await binary_sensor.new_binary_sensor(
            config[CONF_MOTOR_VACUUM_OVERCURRENT_SENSOR]
        )
        cg.add(var.set_vacuum_overcurrent_sensor(sens))
    if CONF_MOTOR_MAINBRUSH_OVERCURRENT_SENSOR in config:
        sens = await binary_sensor.new_binary_sensor(
            config[CONF_MOTOR_MAINBRUSH_OVERCURRENT_SENSOR]
        )
        cg.add(var.set_mainbrush_overcurrent_sensor(sens))
    if CONF_MOTOR_RIGHTWHEEL_OVERCURRENT_SENSOR in config:
        sens = await binary_sensor.new_binary_sensor(
            config[CONF_MOTOR_RIGHTWHEEL_OVERCURRENT_SENSOR]
        )
        cg.add(var.set_rightwheel_overcurrent_sensor(sens))
    if CONF_MOTOR_LEFTWHEEL_OVERCURRENT_SENSOR in config:
        sens = await binary_sensor.new_binary_sensor(
            config[CONF_MOTOR_LEFTWHEEL_OVERCURRENT_SENSOR]
        )
        cg.add(var.set_leftwheel_overcurrent_sensor(sens))
    if CONF_POWER_BUTTON in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_POWER_BUTTON])
        cg.add(var.set_powerbutton_sensor(sens))
    if CONF_SPOT_BUTTON in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_SPOT_BUTTON])
        cg.add(var.set_spotbutton_sensor(sens))
    if CONF_CLEAN_BUTTON in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_CLEAN_BUTTON])
        cg.add(var.set_cleanbutton_sensor(sens))

    roomba = await cg.get_variable(config[CONF_ROOMBA_ID])
    cg.add(roomba.register_listener(var))
