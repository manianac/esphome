from esphome import pins
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import (
    CONF_CLK_PIN,
    CONF_CS_PIN,
    CONF_ID,
    CONF_MISO_PIN,
    CONF_MOSI_PIN,
)
from esphome.core import coroutine

CODEOWNERS = ["@oscillix"]
AUTO_LOAD = ["text_sensor", "sensor", "binary_sensor"]

whirlpool_spi_ns = cg.esphome_ns.namespace("whirlpool_spi")
WhirlpoolSPI = whirlpool_spi_ns.class_("WhirlpoolSPI", cg.PollingComponent)

CONF_WHIRLPOOL_SPI_ID = "whirlpool_spi_id"
CONF_MISO = CONF_MISO_PIN
CONF_MOSI = CONF_MOSI_PIN
CONF_CS = CONF_CS_PIN
CONF_CLK = CONF_CLK_PIN
CONF_SELECTOR_LABELS = "selector_labels"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(WhirlpoolSPI),
        cv.Required(CONF_MISO): pins.internal_gpio_input_pin_schema,
        cv.Required(CONF_MOSI): pins.internal_gpio_input_pin_schema,
        cv.Required(CONF_CS): pins.internal_gpio_input_pin_schema,
        cv.Required(CONF_CLK): pins.internal_gpio_input_pin_schema,
        cv.Optional(CONF_SELECTOR_LABELS, default={}): {
            cv.int_range(min=0, max=15): cv.string,
        },
    }
).extend(cv.polling_component_schema("10s"))


@coroutine
async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    miso_pin = await cg.gpio_pin_expression(config[CONF_MISO])
    cg.add(var.set_miso_pin(miso_pin))

    mosi_pin = await cg.gpio_pin_expression(config[CONF_MOSI])
    cg.add(var.set_mosi_pin(mosi_pin))

    cs_pin = await cg.gpio_pin_expression(config[CONF_CS])
    cg.add(var.set_cs_pin(cs_pin))

    clk_pin = await cg.gpio_pin_expression(config[CONF_CLK])
    cg.add(var.set_clk_pin(clk_pin))

    for index, label in config[CONF_SELECTOR_LABELS].items():
        cg.add(var.set_selector_label(index, label))
