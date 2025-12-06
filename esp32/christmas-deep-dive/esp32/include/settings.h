#ifndef __settings_h__
#define __settings_h__

#include "tools.h"

#define SENSOR_BME280    1

#define SERIAL_SPEED     115200

#define DEVICE_ID        "KOLLBERG"

#define AP_IP_ADDRESS    "192.168.4.1"
#define AP_NET_MASK      "255.255.255.0"
#define AP_WIFI_SSID     getUniqueId().c_str()
#define AP_WIFI_PASSWORD ""

#define MQTT_PORT        1883
#define MQTT_NAME        getUniqueId().c_str()

#endif