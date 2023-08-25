import esphome.config_validation as cv
import esphome.codegen as cg
from esphome import automation
from esphome.components import i2c, time
from esphome.const import CONF_ID


CODEOWNERS = ["@Manianac"]
DEPENDENCIES = ["i2c"]
mcp7940_ns = cg.esphome_ns.namespace("mcp7940")
MCP7940Component = mcp7940_ns.class_(
    "MCP7940Component", time.RealTimeClock, i2c.I2CDevice
)
WriteAction = mcp7940_ns.class_("WriteAction", automation.Action)
ReadAction = mcp7940_ns.class_("ReadAction", automation.Action)

CONF_EXTERNAL_OSC = "external_oscillator"

CONFIG_SCHEMA = time.TIME_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(MCP7940Component),
        cv.Optional(CONF_EXTERNAL_OSC, default="false"): cv.boolean,
    }
).extend(i2c.i2c_device_schema(0x6F))


@automation.register_action(
    "mcp7940.write_time",
    WriteAction,
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(MCP7940Component),
        }
    ),
)
async def mcp7940_write_time_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    return var


@automation.register_action(
    "mcp7940.read_time",
    ReadAction,
    automation.maybe_simple_id(
        {
            cv.GenerateID(): cv.use_id(MCP7940Component),
        }
    ),
)
async def mcp7940_read_time_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    return var


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    cg.add(var.set_external_osc(config[CONF_EXTERNAL_OSC]))
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)
    await time.register_time(var, config)
