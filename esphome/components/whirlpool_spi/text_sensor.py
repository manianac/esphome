import esphome.codegen as cg
from esphome.components import text_sensor
import esphome.config_validation as cv
from esphome.const import ENTITY_CATEGORY_DIAGNOSTIC

from . import CONF_WHIRLPOOL_SPI_ID, WhirlpoolSPI

DEPENDENCIES = ["whirlpool_spi"]

# Config keys for each frame position's MISO and MOSI text sensors
CONF_FRAME0_MISO = "frame0_miso"
CONF_FRAME0_MOSI = "frame0_mosi"
CONF_FRAME1_MISO = "frame1_miso"
CONF_FRAME1_MOSI = "frame1_mosi"
CONF_FRAME2_MISO = "frame2_miso"
CONF_FRAME2_MOSI = "frame2_mosi"
CONF_FRAME3_MISO = "frame3_miso"
CONF_FRAME3_MOSI = "frame3_mosi"
CONF_FRAME4_MISO = "frame4_miso"
CONF_FRAME4_MOSI = "frame4_mosi"
CONF_SELECTOR_POSITION_NAME = "selector_position_name"

# All frame sensor config keys paired with their C++ setter name
FRAME_SENSORS = [
    CONF_FRAME0_MISO,
    CONF_FRAME0_MOSI,
    CONF_FRAME1_MISO,
    CONF_FRAME1_MOSI,
    CONF_FRAME2_MISO,
    CONF_FRAME2_MOSI,
    CONF_FRAME3_MISO,
    CONF_FRAME3_MOSI,
    CONF_FRAME4_MISO,
    CONF_FRAME4_MOSI,
    CONF_SELECTOR_POSITION_NAME,
]

_FRAME_SCHEMA = text_sensor.text_sensor_schema(
    entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(cg.EntityBase),
        cv.GenerateID(CONF_WHIRLPOOL_SPI_ID): cv.use_id(WhirlpoolSPI),
        cv.Optional(CONF_FRAME0_MISO): _FRAME_SCHEMA,
        cv.Optional(CONF_FRAME0_MOSI): _FRAME_SCHEMA,
        cv.Optional(CONF_FRAME1_MISO): _FRAME_SCHEMA,
        cv.Optional(CONF_FRAME1_MOSI): _FRAME_SCHEMA,
        cv.Optional(CONF_FRAME2_MISO): _FRAME_SCHEMA,
        cv.Optional(CONF_FRAME2_MOSI): _FRAME_SCHEMA,
        cv.Optional(CONF_FRAME3_MISO): _FRAME_SCHEMA,
        cv.Optional(CONF_FRAME3_MOSI): _FRAME_SCHEMA,
        cv.Optional(CONF_FRAME4_MISO): _FRAME_SCHEMA,
        cv.Optional(CONF_FRAME4_MOSI): _FRAME_SCHEMA,
        cv.Optional(CONF_SELECTOR_POSITION_NAME): text_sensor.text_sensor_schema(),
    }
)


async def to_code(config):
    hub = await cg.get_variable(config[CONF_WHIRLPOOL_SPI_ID])
    for key in FRAME_SENSORS:
        if sensor_config := config.get(key):
            sens = await text_sensor.new_text_sensor(sensor_config)
            cg.add(getattr(hub, f"set_{key}_text_sensor")(sens))
