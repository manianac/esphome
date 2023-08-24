import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import spi
from esphome.const import (
    CONF_ID,
    CONF_RESOLUTION,
)

DEPENDENCIES = ["spi"]
MULTI_CONF = True
CODEOWNERS = ["@Manianac"]

ads79xx_ns = cg.esphome_ns.namespace("ads79xx")
ADS79XX = ads79xx_ns.class_("ADS79XX", cg.Component, spi.SPIDevice)

ADS79XXResolution = ads79xx_ns.enum("ADS79XXResolution")
RESOLUTION = {
    "12_BITS": ADS79XXResolution.ADS795X_12_BITS,
    "10_BITS": ADS79XXResolution.ADS795X_10_BITS,
    "8_BITS": ADS79XXResolution.ADS79XX_8_BITS,
}

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(ADS79XX),
        cv.Required(CONF_RESOLUTION): cv.enum(RESOLUTION, upper=True, space="_"),
    }
).extend(spi.spi_device_schema(cs_pin_required=True))


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    cg.add(var.set_resolution(config[CONF_RESOLUTION]))
    await cg.register_component(var, config)
    await spi.register_spi_device(var, config)
