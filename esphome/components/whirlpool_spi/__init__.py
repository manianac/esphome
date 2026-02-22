from esphome import pins
import esphome.codegen as cg
from esphome.components import text_sensor
import esphome.config_validation as cv
from esphome.const import (
    CONF_CLK_PIN,
    CONF_CS_PIN,
    CONF_ID,
    CONF_MISO_PIN,
    CONF_MOSI_PIN,
)

CODEOWNERS = ["@oscillix"]
DEPENDENCIES = ["esp32"]

# Create the component namespace
whirlpool_spi_ns = cg.esphome_ns.namespace("whirlpool_spi")
WhirlpoolSPI = whirlpool_spi_ns.class_("WhirlpoolSPI", cg.Component)

# Configuration keys
CONF_BUFFER_SIZE = "buffer_size"
CONF_RX_BUFFER_TEXT_SENSOR = "rx_buffer_text_sensor"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(WhirlpoolSPI),
        cv.Required(CONF_MOSI_PIN): pins.gpio_output_pin_schema,
        cv.Required(CONF_CLK_PIN): pins.gpio_output_pin_schema,
        cv.Optional(CONF_MISO_PIN): pins.gpio_input_pin_schema,
        cv.Required(CONF_CS_PIN): pins.gpio_output_pin_schema,
        cv.Optional(CONF_BUFFER_SIZE, default=256): cv.int_range(min=1, max=4096),
        cv.Optional(CONF_RX_BUFFER_TEXT_SENSOR): text_sensor.text_sensor_schema(),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    # Add logging output
    cg.add(cg.LineComment("Whirlpool SPI Component"))

    # Configure required pins
    mosi_pin = await cg.gpio_pin_expression(config[CONF_MOSI_PIN])
    cg.add(var.set_mosi_pin(mosi_pin))

    clk_pin = await cg.gpio_pin_expression(config[CONF_CLK_PIN])
    cg.add(var.set_clk_pin(clk_pin))

    # Configure optional pins
    if CONF_MISO_PIN in config:
        miso_pin = await cg.gpio_pin_expression(config[CONF_MISO_PIN])
        cg.add(var.set_miso_pin(miso_pin))

    cs_pin = await cg.gpio_pin_expression(config[CONF_CS_PIN])
    cg.add(var.set_cs_pin(cs_pin))

    # Configure buffer size
    cg.add(var.set_buffer_size(config[CONF_BUFFER_SIZE]))

    # Configure optional text sensor for RX buffer
    if CONF_RX_BUFFER_TEXT_SENSOR in config:
        sensor = await text_sensor.new_text_sensor(config[CONF_RX_BUFFER_TEXT_SENSOR])
        cg.add(var.set_rx_buffer_text_sensor(sensor))
