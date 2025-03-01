import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c, sensor
from esphome.const import (
    CONF_CURRENT,
    CONF_ID,
    STATE_CLASS_MEASUREMENT,
    UNIT_MILLIAMP,
    DEVICE_CLASS_CURRENT,
)

DEPENDENCIES = ["i2c"]

acs712_ns = cg.esphome_ns.namespace("acs712")
ACS712Component = acs712_ns.class_("ACS712Component", cg.PollingComponent)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(ACS712Component),
            cv.Optional(CONF_CURRENT): sensor.sensor_schema(
                unit_of_measurement=UNIT_MILLIAMP,
                accuracy_decimals=2,
                device_class=DEVICE_CLASS_CURRENT,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
        }
    )
    .extend(cv.polling_component_schema("60s"))
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    if CONF_CURRENT in config:
        sens = await sensor.new_sensor(config[CONF_CURRENT])
        cg.add(var.set_current_sensor(sens))
