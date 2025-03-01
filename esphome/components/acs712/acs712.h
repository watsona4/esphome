#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"

namespace esphome {
namespace acs712 {

class ACS712Component : public PollingComponent {
 public:
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override;
  void update() override;

  void set_current_sensor(sensor::Sensor *current_sensor) { current_sensor_ = current_sensor; }

 protected:
  bool read_data_(uint8_t *data);

  sensor::Sensor *current_sensor_{nullptr};
};

}  // namespace acs712
}  // namespace esphome
