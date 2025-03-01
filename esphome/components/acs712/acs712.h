#pragma once

#include "esphome/components/sensor/sensor.h"
#include "esphome/core/component.h"

#include "ACS712.h"

namespace esphome {
namespace acs712 {

enum ACS712Model { ACS712_MODEL_5A = 185, ACS712_MODEL_10A = 100, ACS712_MODEL_20A = 66 };

class ACS712Component : public PollingComponent {
 public:
  void setup() override;
  void dump_config() override;
  void update() override;
  float get_setup_priority() const override;

  void set_pin(InternalGPIOPin *pin) { pin_ = pin; }
  void set_model(ACS712Model model) { model_ = model; }
  void set_adc_steps(uint32_t adc_steps) { adc_steps_ = adc_steps; }
  void set_voltage(float voltage) { voltage_ = voltage; }
  void set_current_sensor(sensor::Sensor *current_sensor) { current_sensor_ = current_sensor; }

 protected:
  InternalGPIOPin *pin_;
  uint32_t adc_steps_{1024};
  float voltage_{5.0};
  ACS712Model model_{ACS712_MODEL_5A};
  ACS712 *ACS_{nullptr};
  sensor::Sensor *current_sensor_{nullptr};
};

}  // namespace acs712
}  // namespace esphome
