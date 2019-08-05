#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/esp32_ble_client/esp32_ble_client.h"

#ifdef ARDUINO_ARCH_ESP32

namespace esphome {
namespace meizu_ble {

class MeizuBLE : public PollingComponent, public esp32_ble_client::ESPBTClientListener {
 public:	 
	 void setup() override;
	 void update() override;
	 
     void set_address(uint64_t address) { address_ = address; }
	 
	 void on_client_ready() override;
     void on_connected() override;
     void on_disconnected() override;
	 
     void on_find_service_result(bool result) override;
	 
     void on_char_read(uint8_t *data, uint16_t len) override;
     void on_char_wrote() override;
	 
     void on_error() override;

     void dump_config() override;
     float get_setup_priority() const override { return setup_priority::DATA; }
     void set_temperature(sensor::Sensor *temperature) { temperature_ = temperature; }
     void set_humidity(sensor::Sensor *humidity) { humidity_ = humidity; }
     void set_battery_level(sensor::Sensor *battery_level) { battery_level_ = battery_level; }

    protected:
	 uint8_t sequence_{0};
	 uint8_t client_ready_{0};
     uint64_t address_;
     sensor::Sensor *temperature_{nullptr};
     sensor::Sensor *humidity_{nullptr};
     sensor::Sensor *battery_level_{nullptr};
};

}  // namespace meizu_ble
}  // namespace esphome

#endif
