# Meizu-BLE-Thermometer

ESPHome文件夹是给ESP32+ESPHome使用的，可以把ESP32作为一个蓝牙网关（GATT Client），序列执行读取蓝牙设备，目前做了小米两代蓝牙温度计和魅族蓝牙温度计支持。
使用方法：
1、使用pip安装的esphome:
将ESPHome文件夹内容放到Python的library文件夹的/site-packages/esphome/components/下，在ESPHome配置文件里加入配置，例子在example.yaml里。（可以使用命令： find / -name esp32_ble_tracker 查找文件夹）

2、使用hassio的addon安装的esphome：
使用 docker ps 列出容器，找到esphome容器名，例如hassio的一般叫 addon_15ef4d2f_esphome
进到下载的文件夹，使用 docker cp * addon_15ef4d2f_esphome:/opt/esphome/esphome/components/ 拷贝进容器

3、使用docker安装的，在docker的/usr/src/app/esphome/components/文件夹下。

其中esp32_ble_client为必须，xiaomi_ble_lywsd02对应小米墨水屏温度计（长方形那个），xiaomi_ble_mjhtv1对应小米温度计（圆形有屏那个），meizu_ble对应魅族蓝牙那个。
另外，由于工作原理原因，esp32_ble_client和esp32_ble_tracker是冲突的，不能同时使用。
