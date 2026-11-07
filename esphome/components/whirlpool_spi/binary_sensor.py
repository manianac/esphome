import esphome.codegen as cg
from esphome.components import binary_sensor
import esphome.config_validation as cv
from esphome.types import ConfigType

from . import CONF_WHIRLPOOL_SPI_ID, WhirlpoolSPI

DEPENDENCIES = ["whirlpool_spi"]

# Buttons in the low byte of frame 3 MISO
CONF_CYCLE_SIGNAL = "cycle_signal"
CONF_DELAY_WASH = "delay_wash"
CONF_POWER_BUTTON = "power_button"
CONF_START_PAUSE = "start_pause"

# Buttons in the low byte of frame 4 MISO
CONF_DEEP_CLEAN = "deep_clean"
CONF_DISPENSERS = "dispensers"
CONF_ECOBOOST = "ecoboost"
CONF_EXTRA_RINSE = "extra_rinse"
CONF_SOIL_LEVEL = "soil_level"
CONF_SPIN_SPEED = "spin_speed"
CONF_WASH_TEMP = "wash_temp"

# Panel LEDs in MOSI
CONF_DONE = "done"

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
        cv.Optional(CONF_DONE): binary_sensor.binary_sensor_schema(),
    }
)


async def to_code(config: ConfigType) -> None:
    hub = await cg.get_variable(config[CONF_WHIRLPOOL_SPI_ID])
    buttons = binary_sensor.sub_binary_sensors(config)
    await buttons(CONF_POWER_BUTTON, hub.set_power_button_binary_sensor)
    await buttons(CONF_DELAY_WASH, hub.set_delay_wash_binary_sensor)
    await buttons(CONF_START_PAUSE, hub.set_start_pause_binary_sensor)
    await buttons(CONF_CYCLE_SIGNAL, hub.set_cycle_signal_binary_sensor)
    await buttons(CONF_ECOBOOST, hub.set_ecoboost_binary_sensor)
    await buttons(CONF_EXTRA_RINSE, hub.set_extra_rinse_binary_sensor)
    await buttons(CONF_DEEP_CLEAN, hub.set_deep_clean_binary_sensor)
    await buttons(CONF_SOIL_LEVEL, hub.set_soil_level_binary_sensor)
    await buttons(CONF_SPIN_SPEED, hub.set_spin_speed_binary_sensor)
    await buttons(CONF_WASH_TEMP, hub.set_wash_temp_binary_sensor)
    await buttons(CONF_DISPENSERS, hub.set_dispensers_binary_sensor)
    await buttons(CONF_DONE, hub.set_done_binary_sensor)
