#include "xiaomi_ble_lywsd02.h"
#include "esphome/core/log.h"

#ifdef ARDUINO_ARCH_ESP32

namespace esphome {
namespace xiaomi_ble_lywsd02 {

static const char *TAG = "xiaomi_ble_lywsd02";

void XiaomiBLELYWSD02::setup() {
	
}

void XiaomiBLELYWSD02::update() {
  if (this->client_ready_ == 1) {
    //ESP_LOGD(TAG, "start connect");
 	this->client_->connect(this, this->address_);
  } else {
 	ESP_LOGE(TAG, "client not ready");
  }
}

void XiaomiBLELYWSD02::on_client_ready() {
  //ESP_LOGD(TAG, "Xiaomi lywsd02 on_client_ready");
  this->client_ready_ = 1;
}

void XiaomiBLELYWSD02::on_connected() {
  //ESP_LOGD(TAG, "Xiaomi lywsd02 on_connected");
  this->client_->find_service(this, "EBE0CCB0-7A0A-4B0C-8A1A-6FF2997DA3A6");
}

void XiaomiBLELYWSD02::on_disconnected() {
  //ESP_LOGD(TAG, "Xiaomi lywsd02 on_disconnected");
}

void XiaomiBLELYWSD02::on_find_service_result(bool result) {
  if (result) {
	//ESP_LOGD(TAG, "Xiaomi lywsd02 on_find_service_result true");
	bool char_found = this->client_->find_char(this, "EBE0CCC1-7A0A-4B0C-8A1A-6FF2997DA3A6");
	if (char_found) {
	  //ESP_LOGD(TAG, "Xiaomi lywsd02 on_find_char true");
	  bool descr_found = this->client_->find_char_descr(this, "2902");
	  if (descr_found) {
		//ESP_LOGD(TAG, "Xiaomi lywsd02 on_find_char_descr true");
		this->client_->reg_char_notify(this);
	  } else {
		//ESP_LOGD(TAG, "Xiaomi lywsd02 on_find_char_descr false");
		this->client_->disconnect(this);
	  }
	} else {
	  //ESP_LOGD(TAG, "Xiaomi lywsd02 on_find_char false");
	  this->client_->disconnect(this);
	}
  } else {
	//ESP_LOGD(TAG, "Xiaomi lywsd02 on_find_service_result false");
	this->client_->disconnect(this);
  }
}

void XiaomiBLELYWSD02::on_char_read(uint8_t *data, uint16_t len) {
  //ESP_LOGD(TAG, "Xiaomi lywsd02 on_char_read %d", len);
}

void XiaomiBLELYWSD02::on_char_wrote() {
  //ESP_LOGD(TAG, "Xiaomi lywsd02 on_char_wrote");
}

void XiaomiBLELYWSD02::on_char_descr_wrote() {
  //ESP_LOGD(TAG, "Xiaomi lywsd02 on_char_descr_wrote");
}

void XiaomiBLELYWSD02::on_regged_notify() {
  //ESP_LOGD(TAG, "Xiaomi lywsd02 on_regged_notify");
  uint8_t write_char_data[2] = {0x01, 0x00};
  this->client_->char_descr_write(this, write_char_data, 2, true);
}

void XiaomiBLELYWSD02::on_notify(uint8_t *data, uint16_t len, bool is_notify) {
  if (is_notify){
	if (len == 3) {
	  uint16_t t = 0;
	  t |= (data[1] & 0xff);
	  t<<=8;
	  t |= (data[0] & 0xff);
	  float temp = (float)t / 100.0;
	  uint8_t h = data[2];
	  float humi = (float)h;

	  ESP_LOGD(TAG, "  Temperature: %.1f°C, Humidity: %.1f%%", temp, humi);
	  
	  if (this->temperature_ != nullptr)
	    this->temperature_->publish_state(temp);
	  if (this->humidity_ != nullptr)
	    this->humidity_->publish_state(humi);
	} else {
	  ESP_LOGE(TAG, "Notify result length not match");
	}
	this->client_->disconnect(this);
  }
}

void XiaomiBLELYWSD02::on_error() {
  ESP_LOGE(TAG, "Something wrong with BLE client");
}

void XiaomiBLELYWSD02::dump_config() {
  ESP_LOGCONFIG(TAG, "Xiaomi lywsd02");
  LOG_SENSOR("  ", "Temperature", this->temperature_);
  LOG_SENSOR("  ", "Humidity", this->humidity_);
}

}  // namespace xiaomi_ble_lywsd02
}  // namespace esphome

#endif
