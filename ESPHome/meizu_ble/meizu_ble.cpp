#include "meizu_ble.h"
#include "esphome/core/log.h"

#ifdef ARDUINO_ARCH_ESP32

namespace esphome {
namespace meizu_ble {

static const char *TAG = "meizu_ble";

void MeizuBLE::setup() {
	
}

void MeizuBLE::update() {
  if (this->client_ready_ == 1) {
    //ESP_LOGD(TAG, "Meizu  BLE start connect");
 	this->client_->connect(this, this->address_);
  } else {
 	ESP_LOGE(TAG, "client not ready");
  }
}

void MeizuBLE::on_client_ready() {
  //ESP_LOGD(TAG, "Meizu BLE on_client_ready");
  this->client_ready_ = 1;
}

void MeizuBLE::on_connected() {
  //ESP_LOGD(TAG, "Meizu BLE on_connected");
  this->client_->find_service(this, "16f0");
}

void MeizuBLE::on_disconnected() {
  //ESP_LOGD(TAG, "Meizu BLE on_disconnected");
}

void MeizuBLE::on_find_service_result(bool result) {
  if (result) {
	//ESP_LOGD(TAG, "Meizu BLE on_find_service_result true");
	bool char_found = this->client_->find_char(this, "000016f2-0000-1000-8000-00805f9b34fb");
	if (char_found) {
	  //ESP_LOGD(TAG, "Meizu BLE on_find_char true");
	  uint8_t write_char_data[4] = {0x55, 0x03, this->sequence_, 0x11};
	  this->client_->char_write(this, write_char_data, 4, true);
	  this->sequence_ ++;
	  if (this->sequence_ > 255) {
	  	this->sequence_ = 0;
	  }
	} else {
	  //ESP_LOGD(TAG, "Meizu BLE on_find_char false");
	  this->client_->disconnect(this);
	}
  } else {
	//ESP_LOGD(TAG, "Meizu BLE on_find_service_result false");
	this->client_->disconnect(this);
  }
}

void MeizuBLE::on_char_read(uint8_t *data, uint16_t len) {
  //ESP_LOGD(TAG, "Meizu on_char_read %d", len);
  if (len > 4) {
	if (data[0] == 0x55 && data[1] == 0x07 && data[3] == 0x11 && len == 8) {
	  uint16_t t = 0;
	  t |= (data[5] & 0xff);
	  t<<=8;
	  t |= (data[4] & 0xff);
      float temp = (float)t / 100.0;
      uint16_t h = 0;
	  h |= (data[7] & 0xff);
	  h<<=8;
	  h |= (data[6] & 0xff);
      float humi = (float)h / 100.0;
	  
	  ESP_LOGD(TAG, "  Temperature: %.1f°C, Humidity: %.1f%%", temp, humi);
      if (this->temperature_ != nullptr)
        this->temperature_->publish_state(temp);
      if (this->humidity_ != nullptr)
        this->humidity_->publish_state(humi);
	  
	  uint8_t write_char_data[4] = {0x55, 0x03, this->sequence_, 0x10};
	  this->client_->char_write(this, write_char_data, 4, true);
	  this->sequence_ ++;
	  if (this->sequence_ > 255) {
	  	this->sequence_ = 0;
	  }
	  return;
	} else if (data[0] == 0x55 && data[1] == 0x04 && data[3] == 0x10 && len == 5) {
	  uint8_t v = data[4];
	  float volt = (float)v / 10.0;
	  
	  ESP_LOGD(TAG, "  Battery Voltage: %.1fV", volt);
      if (this->battery_level_ != nullptr)
        this->battery_level_->publish_state(volt);
	} else {
	  ESP_LOGE(TAG, "Unknown read result");
	}
  } else {
  	ESP_LOGE(TAG, "Read result too short");
  }
  this->client_->disconnect(this);
}

void MeizuBLE::on_char_wrote() {
  //ESP_LOGD(TAG, "Meizu BLE on_char_wrote");
  this->client_->char_read(this);
}

void MeizuBLE::on_error() {
  //ESP_LOGE(TAG, "Something wrong with BLE client");
}

void MeizuBLE::dump_config() {
  ESP_LOGCONFIG(TAG, "Meizu BLE");
  LOG_SENSOR("  ", "Temperature", this->temperature_);
  LOG_SENSOR("  ", "Humidity", this->humidity_);
  LOG_SENSOR("  ", "Battery Level", this->battery_level_);
}

}  // namespace meizu_ble
}  // namespace esphome

#endif
