import esphome.codegen as cg
from esphome.components import sensor
import esphome.config_validation as cv
from esphome.const import (
    DEVICE_CLASS_DURATION,
    ENTITY_CATEGORY_DIAGNOSTIC,
    ICON_ROTATE_RIGHT,
    STATE_CLASS_MEASUREMENT,
    STATE_CLASS_TOTAL_INCREASING,
    UNIT_MINUTE,
)
from esphome.types import ConfigType

from . import CONF_WHIRLPOOL_SPI_ID, WhirlpoolSPI

DEPENDENCIES = ["whirlpool_spi"]

CONF_MINUTES_REMAINING = "minutes_remaining"
CONF_SELECTOR_POSITION_INDEX = "selector_position_index"

# Diagnostics, published every update_interval
CONF_BUFFER_OVERFLOWS = "buffer_overflows"
CONF_BUFFER_PEAK = "buffer_peak"
CONF_BUFFER_QUEUE = "buffer_queue"
CONF_ERROR_FRAMES = "error_frames"
CONF_ERROR_OVER = "error_over"
CONF_ERROR_UNDER = "error_under"
CONF_GROUPS_COMPLETED = "groups_completed"
CONF_GROUPS_DROPPED = "groups_dropped"
CONF_GROUPS_FAILED_PLAUSIBILITY = "groups_failed_plausibility"
CONF_TRANSFERS_PER_SEC = "transfers_per_sec"
CONF_VALID_FRAMES = "valid_frames"

_DIAG_SCHEMA = sensor.sensor_schema(
    accuracy_decimals=0,
    entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
    state_class=STATE_CLASS_MEASUREMENT,
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_WHIRLPOOL_SPI_ID): cv.use_id(WhirlpoolSPI),
        cv.Optional(CONF_SELECTOR_POSITION_INDEX): sensor.sensor_schema(
            icon=ICON_ROTATE_RIGHT,
            accuracy_decimals=0,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_MINUTES_REMAINING): sensor.sensor_schema(
            unit_of_measurement=UNIT_MINUTE,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_DURATION,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_TRANSFERS_PER_SEC): _DIAG_SCHEMA,
        cv.Optional(CONF_BUFFER_QUEUE): _DIAG_SCHEMA,
        cv.Optional(CONF_BUFFER_OVERFLOWS): sensor.sensor_schema(
            accuracy_decimals=0,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            state_class=STATE_CLASS_TOTAL_INCREASING,
        ),
        cv.Optional(CONF_BUFFER_PEAK): _DIAG_SCHEMA,
        cv.Optional(CONF_VALID_FRAMES): _DIAG_SCHEMA,
        cv.Optional(CONF_ERROR_FRAMES): _DIAG_SCHEMA,
        cv.Optional(CONF_ERROR_UNDER): _DIAG_SCHEMA,
        cv.Optional(CONF_ERROR_OVER): _DIAG_SCHEMA,
        cv.Optional(CONF_GROUPS_COMPLETED): _DIAG_SCHEMA,
        cv.Optional(CONF_GROUPS_DROPPED): _DIAG_SCHEMA,
        cv.Optional(CONF_GROUPS_FAILED_PLAUSIBILITY): _DIAG_SCHEMA,
    }
)


async def to_code(config: ConfigType) -> None:
    hub = await cg.get_variable(config[CONF_WHIRLPOOL_SPI_ID])
    sensors = sensor.sub_sensors(config)
    await sensors(CONF_SELECTOR_POSITION_INDEX, hub.set_selector_position_sensor)
    await sensors(CONF_MINUTES_REMAINING, hub.set_minutes_remaining_sensor)
    await sensors(CONF_TRANSFERS_PER_SEC, hub.set_transfers_per_sec_sensor)
    await sensors(CONF_BUFFER_QUEUE, hub.set_buffer_queue_sensor)
    await sensors(CONF_BUFFER_OVERFLOWS, hub.set_buffer_overflows_sensor)
    await sensors(CONF_BUFFER_PEAK, hub.set_buffer_peak_sensor)
    await sensors(CONF_VALID_FRAMES, hub.set_valid_frames_sensor)
    await sensors(CONF_ERROR_FRAMES, hub.set_error_frames_sensor)
    await sensors(CONF_ERROR_UNDER, hub.set_error_under_sensor)
    await sensors(CONF_ERROR_OVER, hub.set_error_over_sensor)
    await sensors(CONF_GROUPS_COMPLETED, hub.set_groups_completed_sensor)
    await sensors(CONF_GROUPS_DROPPED, hub.set_groups_dropped_sensor)
    await sensors(
        CONF_GROUPS_FAILED_PLAUSIBILITY, hub.set_groups_failed_plausibility_sensor
    )
