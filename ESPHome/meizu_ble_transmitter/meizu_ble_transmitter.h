#pragma once

#include "esphome/core/component.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/esp32_ble_client/esp32_ble_client.h"

#ifdef ARDUINO_ARCH_ESP32

namespace esphome {
namespace meizu_ble_transmitter {

class MeizuBLETransmitter : public Component, public switch_::Switch, public esp32_ble_client::ESPBTClientListener {
 public:	 
	 void setup() override;
	 void write_state(bool state) override;
	 
     void set_address(uint64_t address) { address_ = address; }
	 
	 void write_next();
	 
	 void on_client_ready() override;
     void on_connected() override;
     void on_disconnected() override;
	 
     void on_find_service_result(bool result) override;
	 
     void on_char_read(uint8_t *data, uint16_t len) override;
     void on_char_wrote() override;
	 
     void on_error() override;

     void dump_config() override;
     float get_setup_priority() const override { return setup_priority::DATA; }
     void set_data_id(const std::string &data_id);
     void set_data(const std::string &data);

    protected:
	 uint8_t sequence_{0};
	 uint8_t client_ready_{0};
     uint64_t address_;
	 std::vector<uint8_t> data_id_;
	 std::vector<uint8_t> data_;
	 std::vector<std::vector<uint8_t>> data_write_;
};

}  // namespace meizu_ble_transmitter
}  // namespace esphome

#endif
