from esphome import pins
import esphome.codegen as cg
from esphome.components import esp32
from esphome.components.esp32 import include_builtin_idf_component
import esphome.config_validation as cv
from esphome.const import (
    CONF_CLK_PIN,
    CONF_CS_PIN,
    CONF_ID,
    CONF_INVERTED,
    CONF_MISO_PIN,
    CONF_MOSI_PIN,
    CONF_NUMBER,
)
from esphome.types import ConfigType

AUTO_LOAD = ["binary_sensor", "sensor", "text_sensor"]
CODEOWNERS = ["@oscillix"]
DOMAIN = "whirlpool_spi"

whirlpool_spi_ns = cg.esphome_ns.namespace("whirlpool_spi")
WhirlpoolSPI = whirlpool_spi_ns.class_("WhirlpoolSPI", cg.PollingComponent)

CONF_CAPTURE_MODE = "capture_mode"
CONF_SELECTOR_LABELS = "selector_labels"
CONF_WHIRLPOOL_SPI_ID = "whirlpool_spi_id"

CAPTURE_GPIO_ISR = "gpio_isr"
CAPTURE_SPI_PERIPHERAL = "spi_peripheral"


def _validate_capture_pin(value: ConfigType) -> ConfigType:
    """Both capture modes read the raw pin level, and the GPIO registers they read only cover GPIO0-31."""
    if value[CONF_NUMBER] > 31:
        raise cv.Invalid("whirlpool_spi pins must be in the range GPIO0-GPIO31")
    if value[CONF_INVERTED]:
        raise cv.Invalid("whirlpool_spi pins cannot be inverted")
    return value


CAPTURE_PIN_SCHEMA = cv.All(pins.internal_gpio_input_pin_schema, _validate_capture_pin)


def _validate_selector_label_indices(value: dict[int, str]) -> dict[int, str]:
    """The selector has positions 0-15."""
    for index in value:
        if not 0 <= index <= 15:
            raise cv.Invalid(f"Selector position {index} is out of range 0-15")
    return value


CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(WhirlpoolSPI),
            cv.Required(CONF_MISO_PIN): CAPTURE_PIN_SCHEMA,
            cv.Required(CONF_MOSI_PIN): CAPTURE_PIN_SCHEMA,
            cv.Required(CONF_CS_PIN): CAPTURE_PIN_SCHEMA,
            cv.Required(CONF_CLK_PIN): CAPTURE_PIN_SCHEMA,
            cv.Optional(CONF_CAPTURE_MODE, default=CAPTURE_GPIO_ISR): cv.one_of(
                CAPTURE_GPIO_ISR, CAPTURE_SPI_PERIPHERAL, lower=True
            ),
            cv.Optional(CONF_SELECTOR_LABELS, default={}): cv.All(
                {cv.int_: cv.string}, _validate_selector_label_indices
            ),
        }
    ).extend(cv.polling_component_schema("10s")),
    cv.only_on_esp32,
    esp32.only_on_variant(supported=[esp32.VARIANT_ESP32]),
)


async def to_code(config: ConfigType) -> None:
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    for conf_key, setter in (
        (CONF_MISO_PIN, var.set_miso_pin),
        (CONF_MOSI_PIN, var.set_mosi_pin),
        (CONF_CS_PIN, var.set_cs_pin),
        (CONF_CLK_PIN, var.set_clk_pin),
    ):
        pin = await cg.gpio_pin_expression(config[conf_key])
        cg.add(setter(pin))

    if config[CONF_CAPTURE_MODE] == CAPTURE_SPI_PERIPHERAL:
        cg.add_define("USE_WHIRLPOOL_SPI_PERIPHERAL")
    else:
        # The CLK glitch filter uses a pulse counter
        include_builtin_idf_component("esp_driver_pcnt")

    for index, label in config[CONF_SELECTOR_LABELS].items():
        cg.add(var.set_selector_label(index, label))
