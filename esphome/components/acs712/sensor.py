from esphome import pins
import esphome.codegen as cg
from esphome.components import sensor
import esphome.config_validation as cv
from esphome.const import (
    CONF_CURRENT,
    CONF_ID,
    CONF_MODEL,
    CONF_PIN,
    CONF_VOLTAGE,
    DEVICE_CLASS_CURRENT,
    STATE_CLASS_MEASUREMENT,
    UNIT_MILLIAMP,
)
from esphome.cpp_helpers import gpio_pin_expression

CONF_ADC_STEPS = "adc_steps"

acs712_ns = cg.esphome_ns.namespace("acs712")
ACS712Model = acs712_ns.enum("ACS712Model")
ACS712_MODELS = {
    "5A": ACS712Model.ACS712_MODEL_5A,
    "10A": ACS712Model.ACS712_MODEL_10A,
    "20A": ACS712Model.ACS712_MODEL_20A,
}
ACS712Component = acs712_ns.class_("ACS712Component", cg.PollingComponent)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(ACS712Component),
        cv.Required(CONF_PIN): pins.internal_gpio_input_pullup_pin_schema,
        cv.Optional(CONF_CURRENT): sensor.sensor_schema(
            unit_of_measurement=UNIT_MILLIAMP,
            accuracy_decimals=2,
            device_class=DEVICE_CLASS_CURRENT,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_MODEL, default="5A"): cv.enum(
            ACS712_MODELS, upper=True, space="_"
        ),
        cv.Optional(CONF_VOLTAGE, default=5.0): cv.positive_float,
        cv.Optional(CONF_ADC_STEPS, default=1024): cv.positive_int,
    }
).extend(cv.polling_component_schema("60s"))


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    pin = await gpio_pin_expression(config[CONF_PIN])
    cg.add(var.set_pin(pin))

    if CONF_CURRENT in config:
        sens = await sensor.new_sensor(config[CONF_CURRENT])
        cg.add(var.set_current_sensor(sens))

    cg.add(var.set_acs712_model(config[CONF_MODEL]))
    cg.add(var.set_voltage(config[CONF_VOLTAGE]))
    cg.add(var.set_adc_steps(config[CONF_ADC_STEPS]))
