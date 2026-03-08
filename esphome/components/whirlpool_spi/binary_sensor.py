import esphome.codegen as cg
from esphome.components import binary_sensor
import esphome.config_validation as cv

from . import CONF_WHIRLPOOL_SPI_ID, WhirlpoolSPI

DEPENDENCIES = ["whirlpool_spi"]

# --- Known button masks (Frame 3 MISO upper byte) ---
CONF_POWER_BUTTON = "power_button"
CONF_DELAY_WASH = "delay_wash"

# --- Buttons with masks still TBD ---
CONF_START_PAUSE = "start_pause"
CONF_CYCLE_SIGNAL = "cycle_signal"
CONF_ECOBOOST = "ecoboost"
CONF_EXTRA_RINSE = "extra_rinse"
CONF_DEEP_CLEAN = "deep_clean"
CONF_SOIL_LEVEL = "soil_level"
CONF_SPIN_SPEED = "spin_speed"
CONF_WASH_TEMP = "wash_temp"
CONF_DISPENSERS = "dispensers"

BUTTON_SENSORS = [
    CONF_POWER_BUTTON,
    CONF_DELAY_WASH,
    CONF_START_PAUSE,
    CONF_CYCLE_SIGNAL,
    CONF_ECOBOOST,
    CONF_EXTRA_RINSE,
    CONF_DEEP_CLEAN,
    CONF_SOIL_LEVEL,
    CONF_SPIN_SPEED,
    CONF_WASH_TEMP,
    CONF_DISPENSERS,
]

_BUTTON_SCHEMA = binary_sensor.binary_sensor_schema()

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_WHIRLPOOL_SPI_ID): cv.use_id(WhirlpoolSPI),
        cv.Optional(CONF_POWER_BUTTON): _BUTTON_SCHEMA,
        cv.Optional(CONF_DELAY_WASH): _BUTTON_SCHEMA,
        cv.Optional(CONF_START_PAUSE): _BUTTON_SCHEMA,
        cv.Optional(CONF_CYCLE_SIGNAL): _BUTTON_SCHEMA,
        cv.Optional(CONF_ECOBOOST): _BUTTON_SCHEMA,
        cv.Optional(CONF_EXTRA_RINSE): _BUTTON_SCHEMA,
        cv.Optional(CONF_DEEP_CLEAN): _BUTTON_SCHEMA,
        cv.Optional(CONF_SOIL_LEVEL): _BUTTON_SCHEMA,
        cv.Optional(CONF_SPIN_SPEED): _BUTTON_SCHEMA,
        cv.Optional(CONF_WASH_TEMP): _BUTTON_SCHEMA,
        cv.Optional(CONF_DISPENSERS): _BUTTON_SCHEMA,
    }
)


async def to_code(config):
    hub = await cg.get_variable(config[CONF_WHIRLPOOL_SPI_ID])
    for key in BUTTON_SENSORS:
        if sensor_config := config.get(key):
            sens = await binary_sensor.new_binary_sensor(sensor_config)
            cg.add(getattr(hub, f"set_{key}_binary_sensor")(sens))
