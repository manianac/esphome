import esphome.codegen as cg
from esphome.components import sensor
import esphome.config_validation as cv
from esphome.const import (
    ENTITY_CATEGORY_DIAGNOSTIC,
    ICON_ROTATE_RIGHT,
    STATE_CLASS_MEASUREMENT,
)

from . import CONF_WHIRLPOOL_SPI_ID, WhirlpoolSPI

DEPENDENCIES = ["whirlpool_spi"]

CONF_SELECTOR_POSITION = "selector_position_index"

# Diagnostic sensor config keys
CONF_TRANSFERS_PER_SEC = "transfers_per_sec"
CONF_BUFFER_QUEUE = "buffer_queue"
CONF_VALID_FRAMES = "valid_frames"
CONF_ERROR_FRAMES = "error_frames"
CONF_ERROR_UNDER = "error_under"
CONF_ERROR_OVER = "error_over"
CONF_GROUPS_COMPLETED = "groups_completed"
CONF_GROUPS_DROPPED = "groups_dropped"
CONF_GROUPS_FAILED_PLAUSIBILITY = "groups_failed_plausibility"

DIAGNOSTIC_SENSORS = [
    CONF_TRANSFERS_PER_SEC,
    CONF_BUFFER_QUEUE,
    CONF_VALID_FRAMES,
    CONF_ERROR_FRAMES,
    CONF_ERROR_UNDER,
    CONF_ERROR_OVER,
    CONF_GROUPS_COMPLETED,
    CONF_GROUPS_DROPPED,
    CONF_GROUPS_FAILED_PLAUSIBILITY,
]

_DIAG_SCHEMA = sensor.sensor_schema(
    accuracy_decimals=0,
    entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
    state_class=STATE_CLASS_MEASUREMENT,
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_WHIRLPOOL_SPI_ID): cv.use_id(WhirlpoolSPI),
        cv.Optional(CONF_SELECTOR_POSITION): sensor.sensor_schema(
            icon=ICON_ROTATE_RIGHT,
            accuracy_decimals=0,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_TRANSFERS_PER_SEC): _DIAG_SCHEMA,
        cv.Optional(CONF_BUFFER_QUEUE): _DIAG_SCHEMA,
        cv.Optional(CONF_VALID_FRAMES): _DIAG_SCHEMA,
        cv.Optional(CONF_ERROR_FRAMES): _DIAG_SCHEMA,
        cv.Optional(CONF_ERROR_UNDER): _DIAG_SCHEMA,
        cv.Optional(CONF_ERROR_OVER): _DIAG_SCHEMA,
        cv.Optional(CONF_GROUPS_COMPLETED): _DIAG_SCHEMA,
        cv.Optional(CONF_GROUPS_DROPPED): _DIAG_SCHEMA,
        cv.Optional(CONF_GROUPS_FAILED_PLAUSIBILITY): _DIAG_SCHEMA,
    }
)


async def to_code(config):
    hub = await cg.get_variable(config[CONF_WHIRLPOOL_SPI_ID])
    if sensor_config := config.get(CONF_SELECTOR_POSITION):
        sens = await sensor.new_sensor(sensor_config)
        cg.add(hub.set_selector_position_sensor(sens))
    for key in DIAGNOSTIC_SENSORS:
        if sensor_config := config.get(key):
            sens = await sensor.new_sensor(sensor_config)
            cg.add(getattr(hub, f"set_{key}_sensor")(sens))
