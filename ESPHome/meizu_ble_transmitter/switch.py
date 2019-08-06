import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import switch, esp32_ble_client
from esphome.const import CONF_MAC_ADDRESS, CONF_ID, CONF_UID, CONF_DATA

DEPENDENCIES = ['esp32_ble_client']

meizu_ble_transmitter_ns = cg.esphome_ns.namespace('meizu_ble_transmitter')
MeizuBLETransmitter = meizu_ble_transmitter_ns.class_('MeizuBLETransmitter', switch.Switch, esp32_ble_client.ESPBTClientListener, cg.Component)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(MeizuBLETransmitter),
    cv.Required(CONF_MAC_ADDRESS): cv.mac_address,
    cv.Required(CONF_UID): cv.string,
    cv.Required(CONF_DATA): cv.string,
}).extend(switch.SWITCH_SCHEMA).extend(esp32_ble_client.ESP_BLE_DEVICE_SCHEMA).extend(cv.COMPONENT_SCHEMA)


def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    yield cg.register_component(var, config)
    yield switch.register_switch(var, config)
    yield esp32_ble_client.register_ble_device(var, config)

    cg.add(var.set_address(config[CONF_MAC_ADDRESS].as_hex))
    cg.add(var.set_data_id(config[CONF_UID]))
    cg.add(var.set_data(config[CONF_DATA]))
