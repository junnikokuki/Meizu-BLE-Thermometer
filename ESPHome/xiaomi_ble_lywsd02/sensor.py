import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor, esp32_ble_client
from esphome.const import CONF_HUMIDITY, CONF_MAC_ADDRESS, CONF_TEMPERATURE, CONF_UPDATE_INTERVAL, \
    UNIT_CELSIUS, UNIT_PERCENT, ICON_THERMOMETER, ICON_WATER_PERCENT, CONF_ID

DEPENDENCIES = ['esp32_ble_client']

xiaomi_ble_lywsd02_ns = cg.esphome_ns.namespace('xiaomi_ble_lywsd02')
XiaomiBLELYWSD02 = xiaomi_ble_lywsd02_ns.class_('XiaomiBLELYWSD02', esp32_ble_client.ESPBTClientListener, cg.Component)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(XiaomiBLELYWSD02),
    cv.Required(CONF_MAC_ADDRESS): cv.mac_address,
    cv.Optional(CONF_TEMPERATURE): sensor.sensor_schema(UNIT_CELSIUS, ICON_THERMOMETER, 1),
    cv.Optional(CONF_HUMIDITY): sensor.sensor_schema(UNIT_PERCENT, ICON_WATER_PERCENT, 1),
    cv.Optional(CONF_UPDATE_INTERVAL, default='180s'): cv.positive_time_period_milliseconds,
}).extend(esp32_ble_client.ESP_BLE_DEVICE_SCHEMA).extend(cv.COMPONENT_SCHEMA)


def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    yield cg.register_component(var, config)
    yield esp32_ble_client.register_ble_device(var, config)

    cg.add(var.set_address(config[CONF_MAC_ADDRESS].as_hex))
    cg.add(var.set_update_interval(config[CONF_UPDATE_INTERVAL]))

    if CONF_TEMPERATURE in config:
        sens = yield sensor.new_sensor(config[CONF_TEMPERATURE])
        cg.add(var.set_temperature(sens))
    if CONF_HUMIDITY in config:
        sens = yield sensor.new_sensor(config[CONF_HUMIDITY])
        cg.add(var.set_humidity(sens))
