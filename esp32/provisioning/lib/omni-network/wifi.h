#ifndef __wifi_h__
#define __wifi_h__

#include <Arduino.h>
#include "network.h"

struct WiFiConfig
{
    String ssid;
    String password;
};

#define IS_WIFI_CONNECTED (WiFi.status() == WL_CONNECTED)

int wifiSetup(networkSetup_t *p_network_setup);
int wifiConfigSave(const WiFiConfig *p_config);
WiFiConfig wifiConfigLoad(void);
void wifiScanNow(void);
String wifiGetScanList(void);
String wifiGetStatus(void);

#endif
