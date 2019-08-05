#include "xiaomi_ble_mjhtv1.h"
#include "esphome/core/log.h"

#ifdef ARDUINO_ARCH_ESP32

namespace esphome {
namespace xiaomi_ble_mjhtv1 {

static const char *TAG = "xiaomi_ble_mjhtv1";

void XiaomiBLEMJHTV1::setup() {
	
}

void XiaomiBLEMJHTV1::update() {
  if (this->client_ready_ == 1) {
    //ESP_LOGD(TAG, "start connect");
 	this->client_->connect(this, this->address_);
  } else {
 	ESP_LOGE(TAG, "client not ready");
  }
}

void XiaomiBLEMJHTV1::on_client_ready() {
  //ESP_LOGD(TAG, "Xiaomi mjhtv1 on_client_ready");
  this->client_ready_ = 1;
}

void XiaomiBLEMJHTV1::on_connected() {
  //ESP_LOGD(TAG, "Xiaomi mjhtv1 on_connected");
  this->client_->find_service(this, "226c0000-6476-4566-7562-66734470666d");
}

void XiaomiBLEMJHTV1::on_disconnected() {
  //ESP_LOGD(TAG, "Xiaomi mjhtv1 on_disconnected");
}

void XiaomiBLEMJHTV1::on_find_service_result(bool result) {
  if (result) {
	//ESP_LOGD(TAG, "Xiaomi mjhtv1 on_find_service_result true");
	bool char_found = this->client_->find_char(this, "226caa55-6476-4566-7562-66734470666d");
	if (char_found) {
	  //ESP_LOGD(TAG, "Xiaomi mjhtv1 on_find_char true");
	  bool descr_found = this->client_->find_char_descr(this, "2902");
	  if (descr_found) {
		//ESP_LOGD(TAG, "Xiaomi mjhtv1 on_find_char_descr true");
		this->client_->reg_char_notify(this);
	  } else {
		//ESP_LOGD(TAG, "Xiaomi mjhtv1 on_find_char_descr false");
		this->client_->disconnect(this);
	  }
	} else {
	  //ESP_LOGD(TAG, "Xiaomi mjhtv1 on_find_char false");
	  this->client_->disconnect(this);
	}
  } else {
	//ESP_LOGD(TAG, "Xiaomi mjhtv1 on_find_service_result false");
	this->client_->disconnect(this);
  }
}

void XiaomiBLEMJHTV1::on_char_read(uint8_t *data, uint16_t len) {
  //ESP_LOGD(TAG, "Xiaomi mjhtv1 on_char_read %d", len);
}

void XiaomiBLEMJHTV1::on_char_wrote() {
  //ESP_LOGD(TAG, "Xiaomi mjhtv1 on_char_wrote");
}

void XiaomiBLEMJHTV1::on_char_descr_wrote() {
  //ESP_LOGD(TAG, "Xiaomi mjhtv1 on_char_descr_wrote");
}

void XiaomiBLEMJHTV1::on_regged_notify() {
  //ESP_LOGD(TAG, "Xiaomi mjhtv1 on_regged_notify");
  uint8_t write_char_data[2] = {0x01, 0x00};
  this->client_->char_descr_write(this, write_char_data, 2, true);
}

void XiaomiBLEMJHTV1::on_notify(uint8_t *data, uint16_t len, bool is_notify) {
  if (is_notify){
	//ESP_LOGD(TAG, "Xiaomi mjhtv1 on_notify %d", len);
	if (len > 7) {
	  std::string dataStr((char*)data, len-1);
	  float temp, humi;
	  if (sscanf(dataStr.c_str(), "T=%f H=%f", &temp, &humi) == 2) {
		ESP_LOGD(TAG, "  Temperature: %.1f°C, Humidity: %.1f%%", temp, humi);
	  
		if (this->temperature_ != nullptr)
		  this->temperature_->publish_state(temp);
		if (this->humidity_ != nullptr)
		  this->humidity_->publish_state(humi);
	  } else {
	  	ESP_LOGE(TAG, "Notify result format not match");
	  }
	} else {
	  ESP_LOGE(TAG, "Notify result length not match");
	}
	this->client_->disconnect(this);
  }
}

void XiaomiBLEMJHTV1::on_error() {
  //ESP_LOGE(TAG, "Something wrong with BLE client");
}

void XiaomiBLEMJHTV1::dump_config() {
  ESP_LOGCONFIG(TAG, "Xiaomi mjhtv1");
  LOG_SENSOR("  ", "Temperature", this->temperature_);
  LOG_SENSOR("  ", "Humidity", this->humidity_);
}

}  // namespace xiaomi_ble_mjhtv1
}  // namespace esphome

#endif
