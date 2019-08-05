#pragma once

#include "esphome/core/component.h"
#include "esphome/core/helpers.h"

#ifdef ARDUINO_ARCH_ESP32

#include <string>
#include <array>
#include <esp_gap_ble_api.h>
#include <esp_bt_defs.h>
#include <esp_gattc_api.h>

namespace esphome {
namespace esp32_ble_client {

class ESP32BLEClient;

class ESPBTClientListener {
 public:
  virtual void on_client_ready() {}
  virtual void on_connected() {};
  virtual void on_disconnected() {};
  virtual void on_find_service_result(bool result) {};
  virtual void on_char_read(uint8_t *data, uint16_t len) {};
  virtual void on_char_wrote() {};
  virtual void on_char_descr_read(uint8_t *data, uint16_t len) {};
  virtual void on_char_descr_wrote() {};
  virtual void on_regged_notify() {};
  virtual void on_notify(uint8_t *data, uint16_t len, bool is_notify) {};
  virtual void on_error() {};
  void set_client(ESP32BLEClient *client) { client_ = client; }

 protected:
  ESP32BLEClient *client_{nullptr};
};

struct gattc_profile_inst {
    ESPBTClientListener *who;
	bool connected;
    uint16_t conn_id;
	esp_bt_uuid_t service_uuid;
    uint16_t service_start_handle;
    uint16_t service_end_handle;
    uint16_t char_handle;
	uint16_t char_descr_handle;
    esp_bd_addr_t remote_bda;
};

typedef enum {
    ESP_CLIENT_STATUS_IDLE = 0,
	ESP_CLIENT_STATUS_CONNECTING = 1,
	ESP_CLIENT_STATUS_CONNECTTED = 2,
	ESP_CLIENT_STATUS_DISCONNECTING = 3
} esp_client_status_t;

class ESP32BLEClient : public Component {
 public:
  /// Setup the FreeRTOS task and the Bluetooth stack.
  void setup() override;
  void dump_config() override;

  void loop() override;

  void register_listener(ESPBTClientListener *listener) {
    listener->set_client(this);
    this->listeners_.push_back(listener);
  }
  
  void set_timeout_interval(uint32_t timeout) { this->timeout_interval_ = timeout; }
  
  void on_error();
  void on_timeout();
  void on_connected();
  void on_disconnected();
  
  void connect(ESPBTClientListener *who, uint64_t mac);
  void connect_first(ESPBTClientListener *who, uint64_t mac);
  void disconnect(ESPBTClientListener *who);
  
  void find_service(ESPBTClientListener *who, std::string uuid);
  bool find_char(ESPBTClientListener *who, std::string uuid);
  bool find_char_descr(ESPBTClientListener *who, std::string uuid);
  
  void char_write(ESPBTClientListener *who, uint8_t *data, uint16_t len, bool response);
  void char_read(ESPBTClientListener *who);
  
  void char_descr_write(ESPBTClientListener *who, uint8_t *data, uint16_t len, bool response);
  void char_descr_read(ESPBTClientListener *who);
  
  void reg_char_notify(ESPBTClientListener *who);

 protected:
  void next_task();
  /// The FreeRTOS task managing the bluetooth interface.
  static bool ble_setup();
  /// Called when gattc event
  static void gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);
  /// Called when gattc events
  void gattc_event_result(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);
  std::vector<ESPBTClientListener *> listeners_;
  SemaphoreHandle_t client_lock_;
  
  uint32_t timeout_interval_{15000};
  
  std::vector<gattc_profile_inst> queue_;
  
  bool cold_start_{true};
  
  uint16_t gattc_if_{0};
  gattc_profile_inst gattc_profile_;
  esp_client_status_t status_{ESP_CLIENT_STATUS_IDLE};
};

extern ESP32BLEClient *global_esp32_ble_client;

}  // namespace esp32_ble_client
}  // namespace esphome

#endif
