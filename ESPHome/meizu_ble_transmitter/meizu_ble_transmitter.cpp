#include "meizu_ble_transmitter.h"
#include "esphome/core/log.h"

#ifdef ARDUINO_ARCH_ESP32

namespace esphome {
namespace meizu_ble_transmitter {

static const char *TAG = "meizu_ble_transmitter";

void MeizuBLETransmitter::set_data_id(const std::string &data_id) {
  std::string str = data_id;
  if (data_id.size() % 2 != 0) {
	str = "0" + data_id;
  }
  for(int i = 0; i < str.size(); i+=2) {
	int val;
  	sscanf(str.substr(i, 2).c_str(), "%2x", &val);
	this->data_id_.push_back((uint8_t)val);
  }
}

void MeizuBLETransmitter::set_data(const std::string &data) {
  std::string str = data;
  if (data.size() % 2 != 0) {
  	str = "0" + data;
  }
  for(int i = 0; i < str.size(); i+=2) {
  	int val;
    sscanf(str.substr(i, 2).c_str(), "%2x", &val);
  	this->data_.push_back((uint8_t)val);
  }
}

void MeizuBLETransmitter::write_next() {
  if (this->data_write_.size() == 0) {
	return;
  }
  std::vector<uint8_t> data = this->data_write_[0];
  uint16_t len = data.size();	
  uint8_t write_char_data[len];
  for (int i = 0; i < len; i++) {
  	write_char_data[i] = data[i];
  }
  this->data_write_.erase(this->data_write_.begin());
  this->client_->char_write(this, write_char_data, len, true);
}

void MeizuBLETransmitter::setup() {
	
}

void MeizuBLETransmitter::write_state(bool state) {
  if (state) {
	if (this->client_ready_ == 1) {
	  //ESP_LOGD(TAG, "start connect");
	  this->client_->connect_first(this, this->address_);
	} else {
	  ESP_LOGE(TAG, "client not ready");
	  this->publish_state(false);
	}
  } else {
  	this->publish_state(false);
  }
}

void MeizuBLETransmitter::on_client_ready() {
  //ESP_LOGD(TAG, "Meizu on_client_ready");
  this->client_ready_ = 1;
}

void MeizuBLETransmitter::on_connected() {
  //ESP_LOGD(TAG, "Meizu on_connected");
  this->client_->find_service(this, "16f0");
}

void MeizuBLETransmitter::on_disconnected() {
  //ESP_LOGD(TAG, "Meizu on_disconnected");
}

void MeizuBLETransmitter::on_find_service_result(bool result) {
  if (result) {
	//ESP_LOGD(TAG, "Meizu on_find_service_result true");
	bool char_found = this->client_->find_char(this, "000016f2-0000-1000-8000-00805f9b34fb");
	if (char_found) {
	  //ESP_LOGD(TAG, "Meizu on_find_char true");
	  uint16_t len = this->data_id_.size();
	  uint8_t write_char_data[4 + len];
	  write_char_data[0] = 0x55;
	  write_char_data[1] = len + 3;
	  write_char_data[2] = this->sequence_;
	  write_char_data[3] = 0x03;
	  for (int i = 0; i < len; i++) {
	  	write_char_data[i + 4] = this->data_id_[i];
	  }
	  this->client_->char_write(this, write_char_data, 4 + len, true);
	} else {
	  //ESP_LOGD(TAG, "Meizu on_find_char false");
	  this->client_->disconnect(this);
	}
  } else {
	//ESP_LOGD(TAG, "Meizu on_find_service_result false");
	this->client_->disconnect(this);
  }
}

void MeizuBLETransmitter::on_char_read(uint8_t *data, uint16_t len) {
  //ESP_LOGD(TAG, "Meizu on_char_read %d", len);  
  if (len >= 4) {
	if (data[0] == 0x55 && data[1] == 0x04 && data[3] == 0x04) {//55 04 xx 04 01
	  if (len == 5 && data[4] == 0x01) {
  	    this->sequence_ ++;
  	    if (this->sequence_ > 255) {
  	  	  this->sequence_ = 0;
  	    }
	  }	else {
		uint16_t packet_count = this->data_.size() / 15 + 1;
		if (this->data_.size() % 15 != 0) {
		  packet_count ++;
		}
		
		uint16_t len = this->data_id_.size();
		std::vector<uint8_t> first_packet;
		first_packet.push_back(0x55);
		first_packet.push_back(len + 5);
		first_packet.push_back(this->sequence_);
		first_packet.push_back(0x00);
		first_packet.push_back(0x00);
		first_packet.push_back(packet_count);
  	    for (int i = 0; i < len; i++) {
  	  	  first_packet.push_back(this->data_id_[i]);
  	    }
		this->data_write_.push_back(first_packet);
		
  	    for (int i = 0; i < packet_count - 1; i++) {
		  uint16_t len = 15;
		  if (this->data_.size() - i * 15 < 15) {
		    len = this->data_.size() - i * 15;
		  }
		  std::vector<uint8_t> packet;
		  packet.push_back(0x55);
		  packet.push_back(len + 4);
		  packet.push_back(this->sequence_);
		  packet.push_back(0x00);
		  packet.push_back(i + 1);
		  for (int j = i * 15; j < (i + 1) * 15; j++) {
		  	packet.push_back(this->data_[j]);
		  }
		  this->data_write_.push_back(packet);
  	    }
		
		this->write_next();
		return;
	  }
    } else if (data[0] == 0x55 && data[1] == 0x03) {// 55 03 xx xx
	  this->sequence_ ++;
	  if (this->sequence_ > 255) {
	  	this->sequence_ = 0;
	  }
    } else {
      ESP_LOGE(TAG, "Read result not match");
    }
  } else {
  	ESP_LOGE(TAG, "Read result too short");
  }
  
  this->client_->disconnect(this);
}

void MeizuBLETransmitter::on_char_wrote() {
  //ESP_LOGD(TAG, "Meizu on_char_wrote");
  if (this->data_write_.size() > 0) {
	this->write_next();
  } else {
  	this->client_->char_read(this);
  }
}

void MeizuBLETransmitter::on_error() {
  //ESP_LOGE(TAG, "Something wrong with BLE client");
}

void MeizuBLETransmitter::dump_config() {
  ESP_LOGCONFIG(TAG, "Meizu BLE Transmitter");
}

}  // namespace meizu_ble_transmitter
}  // namespace esphome

#endif
