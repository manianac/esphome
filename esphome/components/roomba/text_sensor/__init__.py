import esphome.codegen as cg
from esphome.components import text_sensor
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID,
    ICON_BATTERY,
)

from .. import roomba_ns, RoombaComponent, CONF_ROOMBA_ID

AUTO_LOAD = ["text_sensor"]
DEPENDENCIES = ["roomba"]

RoombaTextSensor = roomba_ns.class_(
    "RoombaTextSensor", text_sensor.TextSensor, cg.Component
)

CONF_CHARGING_STATE = "charging_state"
CONF_CHARGING_SOURCE = "charging_source"
CONF_REMOTE_OPCODE = "remote_opcode"

CONFIG_SCHEMA = cv.All(
    cv.COMPONENT_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(RoombaTextSensor),
            cv.GenerateID(CONF_ROOMBA_ID): cv.use_id(RoombaComponent),
            cv.Optional(CONF_CHARGING_STATE): text_sensor.text_sensor_schema(
                icon=ICON_BATTERY
            ),
            cv.GenerateID(): cv.declare_id(RoombaTextSensor),
            cv.GenerateID(CONF_ROOMBA_ID): cv.use_id(RoombaComponent),
            cv.Optional(CONF_CHARGING_SOURCE): text_sensor.text_sensor_schema(
                icon=ICON_BATTERY
            ),
            cv.GenerateID(): cv.declare_id(RoombaTextSensor),
            cv.GenerateID(CONF_ROOMBA_ID): cv.use_id(RoombaComponent),
            cv.Optional(CONF_REMOTE_OPCODE): text_sensor.text_sensor_schema(),
        }
    )
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    if CONF_CHARGING_STATE in config:
        sens = await text_sensor.new_text_sensor(config[CONF_CHARGING_STATE])
        cg.add(var.set_charging_state_text_sensor(sens))

    if CONF_CHARGING_SOURCE in config:
        sens = await text_sensor.new_text_sensor(config[CONF_CHARGING_SOURCE])
        cg.add(var.set_charging_source_text_sensor(sens))

    if CONF_REMOTE_OPCODE in config:
        sens = await text_sensor.new_text_sensor(config[CONF_REMOTE_OPCODE])
        cg.add(var.set_remote_opcode_text_sensor(sens))

    roomba = await cg.get_variable(config[CONF_ROOMBA_ID])
    cg.add(roomba.register_listener(var))
