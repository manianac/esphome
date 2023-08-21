import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import spi
from esphome.const import CONF_ID

DEPENDENCIES = ["spi"]
MULTI_CONF = True
CODEOWNERS = ["@Manianac"]

ads79xx_ns = cg.esphome_ns.namespace("ads79xx")
ADS79XX = ads79xx_ns.class_("ADS79XX", cg.Component, spi.SPIDevice)

CONF_REFERENCE_VOLTAGE = "reference_voltage"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(ADS79XX),
        cv.Optional(CONF_REFERENCE_VOLTAGE, default="5.0V"): cv.voltage,
    }
).extend(spi.spi_device_schema(cs_pin_required=True))


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    cg.add(var.set_reference_voltage(config[CONF_REFERENCE_VOLTAGE]))
    await cg.register_component(var, config)
    await spi.register_spi_device(var, config)
