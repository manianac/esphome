# ESPHome TWAI ISO-TP Component Configuration
# File: components/twai_iso/__init__.py

from esphome import pins
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_BAUDRATE, CONF_ID, CONF_RX_PIN, CONF_TX_PIN

DEPENDENCIES = ["esp32"]
CODEOWNERS = ["@oscillix"]

twai_iso_ns = cg.esphome_ns.namespace("twai_iso")
TWAIISOComponent = twai_iso_ns.class_("TWAIISOComponent", cg.Component)

# Configuration keys
CONF_BUFFER_SIZE = "buffer_size"
CONF_PADDING_BYTE = "padding_byte"
CONF_SLEEP_PIN = "sleep_pin"
CONF_AUTO_SLEEP = "auto_sleep"
CONF_AUTO_SLEEP_TIMEOUT = "auto_sleep_timeout"
CONF_TIMEOUTS = "timeouts"
CONF_FLOW_CONTROL = "flow_control"
CONF_N_AS = "n_as"
CONF_N_AR = "n_ar"
CONF_N_BS = "n_bs"
CONF_N_BR = "n_br"
CONF_N_CS = "n_cs"
CONF_N_CR = "n_cr"
CONF_BLOCK_SIZE = "block_size"
CONF_ST_MIN = "st_min"

# Timeout configuration schema
TIMEOUT_SCHEMA = cv.Schema(
    {
        cv.Optional(CONF_N_AS, default=1000): cv.positive_int,
        cv.Optional(CONF_N_AR, default=1000): cv.positive_int,
        cv.Optional(CONF_N_BS, default=1000): cv.positive_int,
        cv.Optional(CONF_N_BR, default=1000): cv.positive_int,
        cv.Optional(CONF_N_CS, default=0): cv.positive_int,
        cv.Optional(CONF_N_CR, default=1000): cv.positive_int,
    }
)

# Flow control configuration schema
FLOW_CONTROL_SCHEMA = cv.Schema(
    {
        cv.Optional(CONF_BLOCK_SIZE, default=0): cv.int_range(min=0, max=255),
        cv.Optional(CONF_ST_MIN, default=0): cv.int_range(min=0, max=255),
    }
)

# Main component configuration schema
CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(TWAIISOComponent),
        cv.Required(CONF_TX_PIN): pins.gpio_output_pin_schema,
        cv.Required(CONF_RX_PIN): pins.gpio_input_pin_schema,
        cv.Optional(CONF_BAUDRATE, default=500000): cv.one_of(
            125000, 250000, 500000, 800000, 1000000, int=True
        ),
        cv.Optional(CONF_BUFFER_SIZE, default=4095): cv.int_range(min=64, max=4095),
        cv.Optional(CONF_PADDING_BYTE, default=0xAA): cv.int_range(min=0, max=255),
        cv.Optional(CONF_SLEEP_PIN): pins.gpio_output_pin_schema,
        cv.Optional(CONF_AUTO_SLEEP, default=False): cv.boolean,
        cv.Optional(CONF_AUTO_SLEEP_TIMEOUT, default=30000): cv.positive_int,
        cv.Optional(CONF_TIMEOUTS, default={}): TIMEOUT_SCHEMA,
        cv.Optional(CONF_FLOW_CONTROL, default={}): FLOW_CONTROL_SCHEMA,
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    # Basic pins and baudrate
    tx_pin = await cg.gpio_pin_expression(config[CONF_TX_PIN])
    cg.add(var.set_tx_pin(tx_pin))

    rx_pin = await cg.gpio_pin_expression(config[CONF_RX_PIN])
    cg.add(var.set_rx_pin(rx_pin))

    cg.add(var.set_baudrate(config[CONF_BAUDRATE]))
    cg.add(var.set_buffer_size(config[CONF_BUFFER_SIZE]))
    cg.add(var.set_padding_byte(config[CONF_PADDING_BYTE]))

    # Sleep pin configuration
    if CONF_SLEEP_PIN in config:
        sleep_pin = await cg.gpio_pin_expression(config[CONF_SLEEP_PIN])
        cg.add(var.set_sleep_pin(sleep_pin))

    cg.add(var.set_auto_sleep(config[CONF_AUTO_SLEEP]))
    cg.add(var.set_auto_sleep_timeout(config[CONF_AUTO_SLEEP_TIMEOUT]))

    # Timeout configuration
    timeouts = config[CONF_TIMEOUTS]
    cg.add(var.set_timeout_n_as(timeouts[CONF_N_AS]))
    cg.add(var.set_timeout_n_ar(timeouts[CONF_N_AR]))
    cg.add(var.set_timeout_n_bs(timeouts[CONF_N_BS]))
    cg.add(var.set_timeout_n_br(timeouts[CONF_N_BR]))
    cg.add(var.set_timeout_n_cs(timeouts[CONF_N_CS]))
    cg.add(var.set_timeout_n_cr(timeouts[CONF_N_CR]))

    # Flow control configuration
    flow_control = config[CONF_FLOW_CONTROL]
    cg.add(var.set_flow_control_block_size(flow_control[CONF_BLOCK_SIZE]))
    cg.add(var.set_flow_control_st_min(flow_control[CONF_ST_MIN]))
