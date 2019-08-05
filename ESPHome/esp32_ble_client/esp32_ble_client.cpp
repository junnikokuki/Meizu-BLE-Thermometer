#include "esp32_ble_client.h"
#include "esphome/core/log.h"
#include "esphome/core/application.h"

#ifdef ARDUINO_ARCH_ESP32

#include <nvs_flash.h>
#include <freertos/FreeRTOSConfig.h>
#include <esp_bt_main.h>
#include <esp_bt.h>
#include <freertos/task.h>
#include <esp_bt_defs.h>
#include <esp_gattc_api.h>

// bt_trace.h
#undef TAG

namespace esphome {
namespace esp32_ble_client {

static const char *TAG = "esp32_ble_client";

ESP32BLEClient *global_esp32_ble_client = nullptr;

static const uint8_t  base_uuid[ESP_UUID_LEN_128] = {0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
#define UINT32_TO_STREAM(p, u32) {*(p)++ = (uint8_t)(u32); *(p)++ = (uint8_t)((u32) >> 8); *(p)++ = (uint8_t)((u32) >> 16); *(p)++ = (uint8_t)((u32) >> 24);}
#define UINT16_TO_STREAM(p, u16) {*(p)++ = (uint8_t)(u16); *(p)++ = (uint8_t)((u16) >> 8);}

/*******************************************************************************
**
** Function         gatt_convert_uuid16_to_uuid128
**
** Description      Convert a 16 bits UUID to be an standard 128 bits one.
**
** Returns          TRUE if two uuid match; FALSE otherwise.
**
*******************************************************************************/
static void gatt_convert_uuid16_to_uuid128(uint8_t uuid_128[ESP_UUID_LEN_128], uint16_t uuid_16) {
    uint8_t   *p = &uuid_128[ESP_UUID_LEN_128 - 4];
    memcpy (uuid_128, base_uuid, ESP_UUID_LEN_128);
    UINT16_TO_STREAM(p, uuid_16);
}

/*******************************************************************************
**
** Function         gatt_convert_uuid32_to_uuid128
**
** Description      Convert a 32 bits UUID to be an standard 128 bits one.
**
** Returns          TRUE if two uuid match; FALSE otherwise.
**
*******************************************************************************/
static void gatt_convert_uuid32_to_uuid128(uint8_t uuid_128[ESP_UUID_LEN_128], uint32_t uuid_32) {
    uint8_t   *p = &uuid_128[ESP_UUID_LEN_128 - 4];
    memcpy (uuid_128, base_uuid, ESP_UUID_LEN_128);
    UINT32_TO_STREAM(p, uuid_32);
}
/*******************************************************************************
**
** Function         gatt_uuid_compare
**
** Description      Compare two UUID to see if they are the same.
**
** Returns          TRUE if two uuid match; FALSE otherwise.
**
*******************************************************************************/
static bool gatt_uuid_compare (esp_bt_uuid_t src, esp_bt_uuid_t tar) {
    uint8_t  su[ESP_UUID_LEN_128], tu[ESP_UUID_LEN_128];
    uint8_t  *ps, *pt;
    /* any of the UUID is unspecified */
    if (src.len == 0 || tar.len == 0) {
        return true;
    }
    /* If both are 16-bit, we can do a simple compare */
    if (src.len == ESP_UUID_LEN_16 && tar.len == ESP_UUID_LEN_16) {
        return src.uuid.uuid16 == tar.uuid.uuid16;
    }
    /* If both are 32-bit, we can do a simple compare */
    if (src.len == ESP_UUID_LEN_32 && tar.len == ESP_UUID_LEN_32) {
        return src.uuid.uuid32 == tar.uuid.uuid32;
    }
    /* One or both of the UUIDs is 128-bit */
    if (src.len == ESP_UUID_LEN_16) {
        /* convert a 16 bits UUID to 128 bits value */
        gatt_convert_uuid16_to_uuid128(su, src.uuid.uuid16);
        ps = su;
    } else if (src.len == ESP_UUID_LEN_32) {
        gatt_convert_uuid32_to_uuid128(su, src.uuid.uuid32);
        ps = su;
    } else {
        ps = src.uuid.uuid128;
    }
    if (tar.len == ESP_UUID_LEN_16) {
        /* convert a 16 bits UUID to 128 bits value */
        gatt_convert_uuid16_to_uuid128(tu, tar.uuid.uuid16);
        pt = tu;
    } else if (tar.len == ESP_UUID_LEN_32) {
        /* convert a 32 bits UUID to 128 bits value */
        gatt_convert_uuid32_to_uuid128(tu, tar.uuid.uuid32);
        pt = tu;
    } else {
        pt = tar.uuid.uuid128;
    }
    return (memcmp(ps, pt, ESP_UUID_LEN_128) == 0);
}

static esp_bt_uuid_t str2uuid(std::string value) {
	esp_bt_uuid_t uuid;
	if (value.length() == 4) {
		uuid.len         = ESP_UUID_LEN_16;
		int vals[2];
		sscanf(value.c_str(), "%2x%2x",
			&vals[1],
			&vals[0]
		);
		
		uuid.uuid.uuid16 = vals[0] | (vals[1] << 8);
	} else if (value.length() == 8) {
		uuid.len         = ESP_UUID_LEN_32;
		int vals[4];
		sscanf(value.c_str(), "%2x%2x%2x%2x",
			&vals[3],
			&vals[2],
			&vals[1],
			&vals[0]
		);
		
		uuid.uuid.uuid32 = vals[0] | (vals[1] << 8) | (vals[2] << 16) | (vals[3] << 24);
	} else if (value.length() == 36) {
// If the length of the string is 36 bytes then we will assume it is a long hex string in
// UUID format.
		uuid.len = ESP_UUID_LEN_128;
		int vals[16];
		sscanf(value.c_str(), "%2x%2x%2x%2x-%2x%2x-%2x%2x-%2x%2x-%2x%2x%2x%2x%2x%2x",
			&vals[15],
			&vals[14],
			&vals[13],
			&vals[12],
			&vals[11],
			&vals[10],
			&vals[9],
			&vals[8],
			&vals[7],
			&vals[6],
			&vals[5],
			&vals[4],
			&vals[3],
			&vals[2],
			&vals[1],
			&vals[0]
		);

		int i;
		for (i=0; i<16; i++) {
			uuid.uuid.uuid128[i] = vals[i];
		}
	} else {
		ESP_LOGE(TAG, "ERROR: UUID value not 2, 4, 16 or 36 bytes");
	}
	
	return uuid;
}

static void print_uuid(esp_bt_uuid_t uuid) {
  if (uuid.len == ESP_UUID_LEN_16) {
	  ESP_LOGI(TAG, "UUID16: %x", uuid.uuid.uuid16);
  } else if (uuid.len == ESP_UUID_LEN_32) {
	  ESP_LOGI(TAG, "UUID32: %x", uuid.uuid.uuid32);
  } else if (uuid.len == ESP_UUID_LEN_128) {
	  ESP_LOGI(TAG, "UUID128: %02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x", 
	  uuid.uuid.uuid128[15], 
	  uuid.uuid.uuid128[14], 
	  uuid.uuid.uuid128[13], 
	  uuid.uuid.uuid128[12], 
	  uuid.uuid.uuid128[11], 
	  uuid.uuid.uuid128[10], 
	  uuid.uuid.uuid128[9], 
	  uuid.uuid.uuid128[8], 
	  uuid.uuid.uuid128[7], 
	  uuid.uuid.uuid128[6], 
	  uuid.uuid.uuid128[5], 
	  uuid.uuid.uuid128[4], 
	  uuid.uuid.uuid128[3], 
	  uuid.uuid.uuid128[2], 
	  uuid.uuid.uuid128[1], 
	  uuid.uuid.uuid128[0]);
  } else {
  	ESP_LOGI(TAG, "ERROR: UUID value are %d bytes", uuid.len);
  }
}

void ESP32BLEClient::setup() {
  global_esp32_ble_client = this;
  this->client_lock_ = xSemaphoreCreateMutex();

  if (!ESP32BLEClient::ble_setup()) {
    this->mark_failed();
    return;
  }
  
  esp_err_t ret = esp_ble_gattc_app_register(0);
  if (ret) {
    ESP_LOGE(TAG, "ESP-IDF BLE gattc app register failed, error code = %x", ret);
  }
}

void ESP32BLEClient::loop() {

}

bool ESP32BLEClient::ble_setup() {
  // Initialize non-volatile storage for the bluetooth controller
  esp_err_t err = nvs_flash_init();
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "nvs_flash_init failed: %d", err);
    return false;
  }

  // Initialize the bluetooth controller with the default configuration
  if (!btStart()) {
    ESP_LOGE(TAG, "btStart failed: %d", esp_bt_controller_get_status());
    return false;
  }

  esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);

  err = esp_bluedroid_init();
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "esp_bluedroid_init failed: %d", err);
    return false;
  }
  err = esp_bluedroid_enable();
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "esp_bluedroid_enable failed: %d", err);
    return false;
  }
  err = esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_DEFAULT, ESP_PWR_LVL_P7);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "esp_ble_tx_power_set failed: %d", err);
    return false;
  }
  err = esp_ble_gattc_register_callback(ESP32BLEClient::gattc_event_handler);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "esp_ble_gattc_register_callback failed: %d", err);
    return false;
  }

  // BLE takes some time to be fully set up, 200ms should be more than enough
  delay(200);
  
  //ESP_LOGD(TAG, "ble_setup finished");

  return true;
}

void ESP32BLEClient::on_error() {
  if (this->gattc_profile_.who) {
	this->gattc_profile_.who->on_error();
	this->disconnect(this->gattc_profile_.who);
  }
}

void ESP32BLEClient::on_timeout() {
  if (this->gattc_profile_.who) {
    this->disconnect(this->gattc_profile_.who);
  }
}

void ESP32BLEClient::on_connected() {
  if (this->gattc_profile_.who) {
	this->set_timeout("client", this->timeout_interval_, [this]() {
	  ESP_LOGD(TAG, "Client timeout, force to disconnect...");
	  this->on_timeout();
    });
	this->status_ = ESP_CLIENT_STATUS_CONNECTTED;
  	this->gattc_profile_.connected = true;	
  	this->gattc_profile_.who->on_connected();
  }
}

void ESP32BLEClient::on_disconnected() {
  if (this->gattc_profile_.who) {
	this->cancel_timeout("client");
	this->gattc_profile_.connected = false;
	this->gattc_profile_.who->on_disconnected();
	this->gattc_profile_.who = NULL;
	this->status_ = ESP_CLIENT_STATUS_IDLE;
	delay(1000);
	this->next_task();
  }  
}

void ESP32BLEClient::next_task() {
  if (this->queue_.size() == 0) {
	this->cold_start_ = true;
	return;
  }
  while (!xSemaphoreTake(this->client_lock_, 20L / portTICK_PERIOD_MS)) {
	
  }
  
  this->gattc_profile_.who = this->queue_[0].who;
  memcpy(this->gattc_profile_.remote_bda, this->queue_[0].remote_bda, sizeof(esp_bd_addr_t));
  this->queue_.erase(this->queue_.begin());
  
  this->gattc_profile_.connected = false;
  this->gattc_profile_.conn_id = 0;
  this->gattc_profile_.service_start_handle = 0;
  this->gattc_profile_.service_end_handle = 0;
  this->gattc_profile_.char_handle = 0;
  
  this->status_ = ESP_CLIENT_STATUS_CONNECTING;
  
  esp_err_t ret = esp_ble_gattc_open(this->gattc_if_, this->gattc_profile_.remote_bda, (esp_ble_addr_type_t)0, true);
  if (ret) {
    ESP_LOGE(TAG, "ESP-IDF BLE gattc app open failed, error code = %x", ret);
    this->on_error();
  }
  
  xSemaphoreGive(this->client_lock_);
}

void ESP32BLEClient::connect(ESPBTClientListener *who, uint64_t mac) {
  if (who) {
	if (!xSemaphoreTake(this->client_lock_, 20L / portTICK_PERIOD_MS)) {
	  return;
	}

	bool taskExist = false;
	for (unsigned i = 0; i < this->queue_.size(); i++) {
	  if (this->queue_[i].who == who) {
		taskExist = true;
		break;
	  }
	}
	
	if (!taskExist) {
	  gattc_profile_inst new_request;
	  new_request.who = who;
	  esp_bd_addr_t addr;
	  addr[0] = ((mac >> 40) & 0xff);
	  addr[1] = ((mac >> 32) & 0x00ff);
	  addr[2] = ((mac >> 24) & 0x0000ff);
	  addr[3] = ((mac >> 16) & 0x000000ff);
	  addr[4] = ((mac >> 8) & 0x00000000ff);
	  addr[5] = (mac & 0x0000000000ff);
	  memcpy(new_request.remote_bda, addr, sizeof(esp_bd_addr_t));
	  this->queue_.push_back(new_request);
	}
	
	xSemaphoreGive(this->client_lock_);
	if (this->cold_start_) {
	  this->cold_start_ = false;
	  this->next_task();
	}
  }
}

void ESP32BLEClient::connect_first(ESPBTClientListener *who, uint64_t mac) {
  if (who) {
	if (!xSemaphoreTake(this->client_lock_, 20L / portTICK_PERIOD_MS)) {
	  return;
	}

	bool taskExist = false;
	for (unsigned i = 0; i < this->queue_.size(); i++) {
	  if (this->queue_[i].who == who) {
		taskExist = true;
		break;
	  }
	}
	
	if (!taskExist) {
	  gattc_profile_inst new_request;
	  new_request.who = who;
	  esp_bd_addr_t addr;
	  addr[0] = ((mac >> 40) & 0xff);
	  addr[1] = ((mac >> 32) & 0x00ff);
	  addr[2] = ((mac >> 24) & 0x0000ff);
	  addr[3] = ((mac >> 16) & 0x000000ff);
	  addr[4] = ((mac >> 8) & 0x00000000ff);
	  addr[5] = (mac & 0x0000000000ff);
	  memcpy(new_request.remote_bda, addr, sizeof(esp_bd_addr_t));
	  this->queue_.insert(this->queue_.begin(), new_request);
	}
	
	xSemaphoreGive(this->client_lock_);
	if (this->cold_start_) {
	  this->cold_start_ = false;
	  this->next_task();
	}
  }
}

void ESP32BLEClient::disconnect(ESPBTClientListener *who) {
  if (who && who == this->gattc_profile_.who) {
	if (this->status_ == ESP_CLIENT_STATUS_DISCONNECTING) {
	  return;
	}
	this->status_ = ESP_CLIENT_STATUS_DISCONNECTING;
	if (this->gattc_profile_.connected) {
	  esp_err_t ret = esp_ble_gattc_close(this->gattc_if_, this->gattc_profile_.conn_id);
	  if (ret == ESP_OK) {
		  return;
	  } else {
	    ESP_LOGE(TAG, "ESP-IDF BLE gattc app close failed, error code = %x", ret);
	  }
	}
	
    this->on_disconnected();
  }
}

void ESP32BLEClient::find_service(ESPBTClientListener *who, std::string uuid) {
  if (who && who == this->gattc_profile_.who) {
	this->gattc_profile_.service_start_handle = 0;
	this->gattc_profile_.service_end_handle = 0;
	this->gattc_profile_.char_handle = 0;
	this->gattc_profile_.char_descr_handle = 0;
	this->gattc_profile_.service_uuid = str2uuid(uuid);
	esp_err_t ret = esp_ble_gattc_search_service(this->gattc_if_, this->gattc_profile_.conn_id, NULL);
	if (ret) {
	  ESP_LOGE(TAG, "ESP-IDF BLE gattc app search service failed, error code = %x", ret);
	  this->on_error();
	}
  }
}

bool ESP32BLEClient::find_char(ESPBTClientListener *who, std::string uuid) {
  bool found = false;
  if (who && who == this->gattc_profile_.who) {
  	this->gattc_profile_.char_handle = 0;
  	this->gattc_profile_.char_descr_handle = 0;
	uint16_t count = 0;
	esp_gatt_status_t status = esp_ble_gattc_get_attr_count(this->gattc_if_,
    											this->gattc_profile_.conn_id,
												ESP_GATT_DB_CHARACTERISTIC,
    											this->gattc_profile_.service_start_handle,
    											this->gattc_profile_.service_end_handle,
												0,
												&count);
	if (status != ESP_GATT_OK) {
	  ESP_LOGE(TAG, "esp_ble_gattc_get_attr_count error %d", status);
	}
    if (count > 0) {
      esp_bt_uuid_t filter_uuid = str2uuid(uuid);
      esp_gattc_char_elem_t *char_elem_result = (esp_gattc_char_elem_t *)malloc(sizeof(esp_gattc_char_elem_t)*count);
      if (!char_elem_result) {
        ESP_LOGE(TAG, "gattc no mem");
      } else {
		/*
		uint16_t offset = 0;
		esp_gatt_status_t status = esp_ble_gattc_get_all_char(this->gattc_if_,
        										  this->gattc_profile_.conn_id,
        										  this->gattc_profile_.service_start_handle,
        										  this->gattc_profile_.service_end_handle,
                                                  char_elem_result,
                                                  &count,
                                                  offset);
        if (status != ESP_GATT_OK) {
          ESP_LOGE(TAG, "esp_ble_gattc_get_all_char error, %d", status);
        } else {
		  if (count > 0) {
			for (int i = 0; i < count; ++i) {
		  	  print_uuid(char_elem_result[i].uuid);
			}
		  }
        }
		*/
        esp_gatt_status_t status = esp_ble_gattc_get_char_by_uuid(this->gattc_if_,
                                                   this->gattc_profile_.conn_id,
                                                   this->gattc_profile_.service_start_handle,
                                                   this->gattc_profile_.service_end_handle,
                                                   filter_uuid,
                                                   char_elem_result,
                                                   &count);
        if (status == ESP_GATT_OK) {
  		  this->gattc_profile_.char_handle = char_elem_result[0].char_handle;
  		  found = true;
  	    } else {
          ESP_LOGE(TAG, "esp_ble_gattc_get_char_by_uuid error %d", status);
        }
      }
      /* free char_elem_result */
      free(char_elem_result);
	}
  }
  return found;
}

bool ESP32BLEClient::find_char_descr(ESPBTClientListener *who, std::string uuid) {
  bool found = false;
  if (who && who == this->gattc_profile_.who) {
	this->gattc_profile_.char_descr_handle = 0;
  	uint16_t count = 0;
  	esp_gatt_status_t status = esp_ble_gattc_get_attr_count(this->gattc_if_,
      											this->gattc_profile_.conn_id,
  												ESP_GATT_DB_DESCRIPTOR,
      											this->gattc_profile_.service_start_handle,
      											this->gattc_profile_.service_end_handle,
  												this->gattc_profile_.char_handle,
  												&count);
  	if (status != ESP_GATT_OK) {
  	  ESP_LOGE(TAG, "esp_ble_gattc_get_attr_count error %d", status);
  	}
    if (count > 0) {
      esp_bt_uuid_t filter_uuid = str2uuid(uuid);
      esp_gattc_descr_elem_t *descr_elem_result = (esp_gattc_descr_elem_t *)malloc(sizeof(esp_gattc_descr_elem_t)*count);
      if (!descr_elem_result) {
        ESP_LOGE(TAG, "gattc no mem");
      } else {
  		/*
  		uint16_t offset = 0;
  		esp_gatt_status_t status = esp_ble_gattc_get_all_descr(this->gattc_if_,
          										  this->gattc_profile_.conn_id,
          										  this->gattc_profile_.char_handle,
                                                  descr_elem_result,
                                                  &count,
                                                  offset);
        if (status != ESP_GATT_OK) {
          ESP_LOGE(TAG, "esp_ble_gattc_get_all_descr error, %d", status);
        } else {
  		  if (count > 0) {
  			for (int i = 0; i < count; ++i) {
  		  	  print_uuid(descr_elem_result[i].uuid);
  			}
  		  }
        }
  		*/
        esp_gatt_status_t status = esp_ble_gattc_get_descr_by_char_handle(this->gattc_if_,
                                                     this->gattc_profile_.conn_id,
                                                     this->gattc_profile_.char_handle,
                                                     filter_uuid,
                                                     descr_elem_result,
                                                     &count);
        if (status == ESP_GATT_OK) {
    	  this->gattc_profile_.char_descr_handle = descr_elem_result[0].handle;
    	  found = true;
    	} else {
          ESP_LOGE(TAG, "esp_ble_gattc_get_descr_by_char_handle error %d", status);
        }
      }
      /* free char_elem_result */
      free(descr_elem_result);
  	}
  }
  return found;
}

void ESP32BLEClient::char_write(ESPBTClientListener *who, uint8_t *data, uint16_t len, bool response) {
  if (who && who == this->gattc_profile_.who) {
	esp_err_t ret = esp_ble_gattc_write_char(this->gattc_if_,
	                                    this->gattc_profile_.conn_id,
	                                    this->gattc_profile_.char_handle,
	                                    len,
	                                    data,
	                                    response ? ESP_GATT_WRITE_TYPE_RSP : ESP_GATT_WRITE_TYPE_NO_RSP,
	                                    ESP_GATT_AUTH_REQ_NONE);
  	if (ret) {
  	  ESP_LOGE(TAG, "ESP-IDF BLE gattc app char write failed, error code = %x", ret);
  	  this->on_error();
  	}
  }
}

void ESP32BLEClient::char_read(ESPBTClientListener *who) {
  if (who && who == this->gattc_profile_.who) {
  	esp_err_t ret = esp_ble_gattc_read_char(this->gattc_if_,
  	                                    this->gattc_profile_.conn_id,
  	                                    this->gattc_profile_.char_handle,
  	                                    ESP_GATT_AUTH_REQ_NONE);
    if (ret) {
      ESP_LOGE(TAG, "ESP-IDF BLE gattc app char read failed, error code = %x", ret);
      this->on_error();
    }
  }
}

void ESP32BLEClient::char_descr_write(ESPBTClientListener *who, uint8_t *data, uint16_t len, bool response) {
  if (who && who == this->gattc_profile_.who) {
	esp_err_t ret = esp_ble_gattc_write_char_descr(this->gattc_if_,
	                                    this->gattc_profile_.conn_id,
	                                    this->gattc_profile_.char_descr_handle,
	                                    len,
	                                    data,
	                                    response ? ESP_GATT_WRITE_TYPE_RSP : ESP_GATT_WRITE_TYPE_NO_RSP,
	                                    ESP_GATT_AUTH_REQ_NONE);
  	if (ret) {
  	  ESP_LOGE(TAG, "ESP-IDF BLE gattc app char descr write failed, error code = %x", ret);
  	  this->on_error();
  	}
  }
}

void ESP32BLEClient::char_descr_read(ESPBTClientListener *who) {
  if (who && who == this->gattc_profile_.who) {
  	esp_err_t ret = esp_ble_gattc_read_char_descr(this->gattc_if_,
  	                                    this->gattc_profile_.conn_id,
  	                                    this->gattc_profile_.char_descr_handle,
  	                                    ESP_GATT_AUTH_REQ_NONE);
    if (ret) {
      ESP_LOGE(TAG, "ESP-IDF BLE gattc app char descr read failed, error code = %x", ret);
      this->on_error();
    }
  }
}

void ESP32BLEClient::reg_char_notify(ESPBTClientListener *who) {
  if (who && who == this->gattc_profile_.who) {
	esp_err_t ret = esp_ble_gattc_register_for_notify (this->gattc_if_, this->gattc_profile_.remote_bda, this->gattc_profile_.char_handle);
    if (ret) {
      ESP_LOGE(TAG, "ESP-IDF BLE gattc app char reg notify failed, error code = %x", ret);
      this->on_error();
    }
  }
}

void ESP32BLEClient::gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param) {
  global_esp32_ble_client->gattc_event_result(event, gattc_if, param);
}

void ESP32BLEClient::gattc_event_result(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param) {
  esp_ble_gattc_cb_param_t *p_data = (esp_ble_gattc_cb_param_t *)param;
  switch (event) {
	case ESP_GATTC_REG_EVT: {
	  //ESP_LOGD(TAG, "REG_EVT");
      if (param->reg.status == ESP_GATT_OK) {
		this->gattc_if_ = gattc_if;
		for (auto * listener : this->listeners_) {
		  listener->on_client_ready();
		}
      } else {
        ESP_LOGE(TAG, "ESP-IDF BLE gattc app register callback failed, error code = %x", param->reg.status);
      }
	  break;
    }
    case ESP_GATTC_CONNECT_EVT: {
      //ESP_LOGD(TAG, "ESP_GATTC_CONNECT_EVT conn_id %d, if %d", p_data->connect.conn_id, gattc_if);
      this->gattc_profile_.conn_id = p_data->connect.conn_id;
      esp_err_t mtu_ret = esp_ble_gattc_send_mtu_req(gattc_if, p_data->connect.conn_id);
      if (mtu_ret) {
        ESP_LOGE(TAG, "config MTU error, error code = %x", mtu_ret);
      }
      break;
    }
    case ESP_GATTC_OPEN_EVT: {
	  //ESP_LOGD(TAG, "ESP_GATTC_OPEN_EVT");
      if (param->open.status == ESP_GATT_OK) {
	    this->on_connected();
      } else {
		ESP_LOGE(TAG, "open failed, status %d", p_data->open.status);
		//this->on_error();
	  }
      break;
	}
	case ESP_GATTC_SEARCH_CMPL_EVT: {
	  //ESP_LOGD(TAG, "ESP_GATTC_SEARCH_CMPL_EVT");
	  if (p_data->search_cmpl.status == ESP_GATT_OK){
		if (this->gattc_profile_.service_start_handle == 0 && this->gattc_profile_.service_end_handle == 0) {
		  this->gattc_profile_.who->on_find_service_result(false);
		} else {
		  this->gattc_profile_.who->on_find_service_result(true);
		}
	  } else {
	  	ESP_LOGE(TAG, "search service failed, error status = %x", p_data->search_cmpl.status);
		this->on_error();
	  }
	  break;
	}
	case ESP_GATTC_SEARCH_RES_EVT: {
	  //ESP_LOGD(TAG, "ESP_GATTC_SEARCH_RES_EVT");
	  //print_uuid(p_data->search_res.srvc_id.uuid);
	  if (gatt_uuid_compare(p_data->search_res.srvc_id.uuid, this->gattc_profile_.service_uuid)) {
		this->gattc_profile_.service_start_handle = p_data->search_res.start_handle;
		this->gattc_profile_.service_end_handle = p_data->search_res.end_handle;
	  }
	  break;
	}
	case ESP_GATTC_READ_CHAR_EVT: {
  	  //ESP_LOGD(TAG, "ESP_GATTC_READ_CHAR_EVT");
      if (p_data->read.status == ESP_GATT_OK){
    	this->gattc_profile_.who->on_char_read(p_data->read.value, p_data->read.value_len);
      } else {
    	ESP_LOGE(TAG, "char read failed, error status = %x", p_data->read.status);
    	this->on_error();
      }
	  break;
	}
	case ESP_GATTC_WRITE_CHAR_EVT: {
	  //ESP_LOGD(TAG, "ESP_GATTC_WRITE_CHAR_EVT");
  	  if (p_data->write.status == ESP_GATT_OK){
  		this->gattc_profile_.who->on_char_wrote();
  	  } else {
  	  	ESP_LOGE(TAG, "char write failed, error status = %x", p_data->write.status);
  		this->on_error();
  	  }
	  break;
	}
	case ESP_GATTC_REG_FOR_NOTIFY_EVT: {
	  //ESP_LOGD(TAG, "ESP_GATTC_REG_FOR_NOTIFY_EVT");
	  if (p_data->reg_for_notify.status == ESP_GATT_OK) {
		  this->gattc_profile_.who->on_regged_notify();
	  } else {
        ESP_LOGE(TAG, "char reg notify failed, error status = %x", p_data->write.status);
    	this->on_error();
	  }
	}
	case ESP_GATTC_NOTIFY_EVT: {
	  //ESP_LOGD(TAG, "ESP_GATTC_NOTIFY_EVT");
	  this->gattc_profile_.who->on_notify(p_data->notify.value, p_data->notify.value_len, p_data->notify.is_notify);
	  break;
	}
	case ESP_GATTC_READ_DESCR_EVT: {
      //ESP_LOGD(TAG, "ESP_GATTC_READ_DESCR_EVT");
      if (p_data->read.status == ESP_GATT_OK){
      	this->gattc_profile_.who->on_char_descr_read(p_data->read.value, p_data->read.value_len);
      } else {
      	ESP_LOGE(TAG, "char descr read failed, error status = %x", p_data->read.status);
      	this->on_error();
      }
  	  break;
	}
	case ESP_GATTC_WRITE_DESCR_EVT: {
  	  //ESP_LOGD(TAG, "ESP_GATTC_WRITE_DESCR_EVT");
      if (p_data->write.status == ESP_GATT_OK){
    	this->gattc_profile_.who->on_char_descr_wrote();
      } else {
    	ESP_LOGE(TAG, "char descr write failed, error status = %x", p_data->write.status);
    	this->on_error();
      }
  	  break;
	}
	case ESP_GATTC_CLOSE_EVT: {
	  //ESP_LOGD(TAG, "ESP_GATTC_CLOSE_EVT status %d, reason %d", p_data->close.status, p_data->close.reason);
	  break;
	}
	case ESP_GATTC_DISCONNECT_EVT: {
	  //ESP_LOGD(TAG, "ESP_GATTC_DISCONNECT_EVT conn_id %d, if %d, reason %d", p_data->disconnect.conn_id, gattc_if, p_data->disconnect.reason);
	  this->on_disconnected();
	  break;
	}
	default:
	  break;
  }
}

void ESP32BLEClient::dump_config() {
  ESP_LOGCONFIG(TAG, "BLE Client:");
}

}  // namespace esp32_ble_client
}  // namespace esphome

#endif
