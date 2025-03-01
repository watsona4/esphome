// Implementation based on:
//  - ACS712_sensor_library: https://github.com/marianomd/acs712-esphome/blob/main/acs712_component.h

#include "acs712.h"
#include "esphome/core/log.h"

namespace esphome {
namespace acs712 {

static const char *const TAG = "acs712";

void ACS712Component::update() {
  uint8_t data[5];
  if (!this->read_data_(data)) {
    this->status_set_warning();
    return;
  }
  const uint16_t raw_temperature = uint16_t(data[2]) * 10 + (data[3] & 0x7F);
  float temperature = raw_temperature / 10.0f;
  if ((data[3] & 0x80) != 0) {
    // negative
    temperature *= -1;
  }

  const uint16_t raw_humidity = uint16_t(data[0]) * 10 + data[1];
  float humidity = raw_humidity / 10.0f;

  ESP_LOGD(TAG, "Got temperature=%.2f°C humidity=%.2f%%", temperature, humidity);
  if (this->temperature_sensor_ != nullptr)
    this->temperature_sensor_->publish_state(temperature);
  if (this->humidity_sensor_ != nullptr)
    this->humidity_sensor_->publish_state(humidity);
  this->status_clear_warning();
}
void ACS712Component::setup() {
  ESP_LOGCONFIG(TAG, "Setting up ACS712...");
  uint8_t data[5];
  if (!this->read_data_(data)) {
    this->mark_failed();
    return;
  }
}
void ACS712Component::dump_config() {
  ESP_LOGD(TAG, "ACS712:");
  LOG_I2C_DEVICE(this);
  if (this->is_failed()) {
    ESP_LOGE(TAG, "Communication with ACS712 failed!");
  }
  LOG_SENSOR("  ", "Temperature", this->temperature_sensor_);
  LOG_SENSOR("  ", "Humidity", this->humidity_sensor_);
}
float ACS712Component::get_setup_priority() const { return setup_priority::DATA; }
bool ACS712Component::read_data_(uint8_t *data) {
  if (!this->read_bytes(0, data, 5)) {
    ESP_LOGW(TAG, "Updating ACS712 failed!");
    return false;
  }

  uint8_t checksum = data[0] + data[1] + data[2] + data[3];
  if (data[4] != checksum) {
    ESP_LOGW(TAG, "ACS712 Checksum invalid!");
    return false;
  }

  return true;
}

}  // namespace acs712
}  // namespace esphome
