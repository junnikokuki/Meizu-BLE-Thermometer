import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_TIMEOUT, ESP_PLATFORM_ESP32
from esphome.core import coroutine

ESP_PLATFORMS = [ESP_PLATFORM_ESP32]

CONF_ESP32_BLE_ID = 'esp32_ble_id'
esp32_ble_client_ns = cg.esphome_ns.namespace('esp32_ble_client')
ESP32BLEClient = esp32_ble_client_ns.class_('ESP32BLEClient', cg.Component)
ESPBTClientListener = esp32_ble_client_ns.class_('ESPBTClientListener')

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(ESP32BLEClient),
    cv.Optional(CONF_TIMEOUT, default='15s'): cv.positive_time_period_milliseconds,
}).extend(cv.COMPONENT_SCHEMA)

ESP_BLE_DEVICE_SCHEMA = cv.Schema({
    cv.GenerateID(CONF_ESP32_BLE_ID): cv.use_id(ESP32BLEClient),
})


def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    yield cg.register_component(var, config)
    cg.add(var.set_timeout_interval(config[CONF_TIMEOUT]))


@coroutine
def register_ble_device(var, config):
    paren = yield cg.get_variable(config[CONF_ESP32_BLE_ID])
    cg.add(paren.register_listener(var))
    yield var
