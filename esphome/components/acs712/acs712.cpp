// Implementation based on:
//  - ACS712_sensor_library:
//  https://github.com/marianomd/acs712-esphome/blob/main/acs712_component.h
//  https://github.com/RobTillaart/ACS712/blob/master/ACS712.h
//  https://github.com/RobTillaart/ACS712/blob/master/ACS712.cpp

#include "acs712.h"
#include "esphome/core/log.h"

namespace esphome {
namespace acs712 {

static const char *const TAG = "acs712";

void ACS712Component::setup() {
  ESP_LOGCONFIG(TAG, "Setting up ACS712...");
  this->pin_->digital_write(true);
  this->pin_->setup();
  this->pin_->digital_write(true);
  ACS_ = new ACS712(this->pin_->get_pin(), this->voltage_, this->adc_steps_, this->mVperA_);
  ACS_->autoMidPoint();
  ACS_->setNoisemV(43);
}
void ACS712Component::dump_config() {
  ESP_LOGCONFIG(TAG, "ACS712:");
  ESP_LOGCONFIG(TAG, "  mV per A: %.1f", this->mVperA_);
  ESP_LOGCONFIG(TAG, "  ADC Steps: %d", this->adc_steps_);
  ESP_LOGCONFIG(TAG, "  Voltage: %.1f", this->voltage_);
  LOG_PIN("  Pin: ", this->pin_);
  ESP_LOGCONFIG(TAG, "  Internal Pull-up: %s", ONOFF(this->pin_->get_flags() & gpio::FLAG_PULLUP));

  LOG_UPDATE_INTERVAL(this);

  LOG_SENSOR("  ", "Current", this->current_sensor_);
}
void ACS712Component::update() {
  float average = 0;
  int count = 5;
  for (int i = 0; i < count; i++) {
    average += ACS_->mA_AC();
  }
  float current = average / count / 1000.0;

  ESP_LOGD(TAG, "Got current=%.2f mA", current);

  if (this->current_sensor_ != nullptr)
    this->current_sensor_->publish_state(current);
  this->status_clear_warning();
}
float ACS712Component::get_setup_priority() const { return setup_priority::DATA; }

}  // namespace acs712
}  // namespace esphome
