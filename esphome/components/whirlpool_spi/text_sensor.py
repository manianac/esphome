import esphome.codegen as cg
from esphome.components import text_sensor
import esphome.config_validation as cv
from esphome.const import CONF_DISPLAY, ENTITY_CATEGORY_DIAGNOSTIC
from esphome.types import ConfigType

from . import CONF_WHIRLPOOL_SPI_ID, WhirlpoolSPI

DEPENDENCIES = ["whirlpool_spi"]

# Raw MISO and MOSI words for each frame of a group, as hex
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

_FRAME_SCHEMA = text_sensor.text_sensor_schema(
    entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
)

CONFIG_SCHEMA = cv.Schema(
    {
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
        cv.Optional(CONF_DISPLAY): text_sensor.text_sensor_schema(),
    }
)


async def to_code(config: ConfigType) -> None:
    hub = await cg.get_variable(config[CONF_WHIRLPOOL_SPI_ID])
    sensors = text_sensor.sub_text_sensors(config)
    await sensors(CONF_FRAME0_MISO, hub.set_frame0_miso_text_sensor)
    await sensors(CONF_FRAME0_MOSI, hub.set_frame0_mosi_text_sensor)
    await sensors(CONF_FRAME1_MISO, hub.set_frame1_miso_text_sensor)
    await sensors(CONF_FRAME1_MOSI, hub.set_frame1_mosi_text_sensor)
    await sensors(CONF_FRAME2_MISO, hub.set_frame2_miso_text_sensor)
    await sensors(CONF_FRAME2_MOSI, hub.set_frame2_mosi_text_sensor)
    await sensors(CONF_FRAME3_MISO, hub.set_frame3_miso_text_sensor)
    await sensors(CONF_FRAME3_MOSI, hub.set_frame3_mosi_text_sensor)
    await sensors(CONF_FRAME4_MISO, hub.set_frame4_miso_text_sensor)
    await sensors(CONF_FRAME4_MOSI, hub.set_frame4_mosi_text_sensor)
    await sensors(
        CONF_SELECTOR_POSITION_NAME, hub.set_selector_position_name_text_sensor
    )
    await sensors(CONF_DISPLAY, hub.set_display_text_sensor)
