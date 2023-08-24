import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor, voltage_sampler
from esphome.const import CONF_ID, CONF_CHANNEL

from .. import ads79xx_ns, ADS79XX

from esphome.const import (
    DEVICE_CLASS_VOLTAGE,
    STATE_CLASS_MEASUREMENT,
    UNIT_VOLT,
)

AUTO_LOAD = ["voltage_sampler"]
DEPENDENCIES = ["ads79xx"]

ADS79XXSensor = ads79xx_ns.class_(
    "ADS79XXSensor",
    sensor.Sensor,
    cg.PollingComponent,
    voltage_sampler.VoltageSampler,
)
CONF_ADS79XX_ID = "ads79xx_id"
CONF_DOUBLE_VREF = "double_vref"

CONFIG_SCHEMA = (
    sensor.sensor_schema(
        ADS79XXSensor,
        unit_of_measurement=UNIT_VOLT,
        accuracy_decimals=3,
        device_class=DEVICE_CLASS_VOLTAGE,
        state_class=STATE_CLASS_MEASUREMENT,
    )
    .extend(
        {
            cv.GenerateID(CONF_ADS79XX_ID): cv.use_id(ADS79XX),
            cv.Required(CONF_CHANNEL): cv.int_range(min=0, max=15),
            cv.Optional(CONF_DOUBLE_VREF, default="true"): cv.boolean,
        }
    )
    .extend(cv.polling_component_schema("60s"))
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    cg.add(var.set_channel(config[CONF_CHANNEL]))
    cg.add(var.set_double_vref(config[CONF_DOUBLE_VREF]))
    await cg.register_parented(var, config[CONF_ADS79XX_ID])
    await cg.register_component(var, config)
    await sensor.register_sensor(var, config)
