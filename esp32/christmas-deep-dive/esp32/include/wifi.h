#ifndef __wifi_h__
#define __wifi_h__

#include <WiFi.h>

void wifiSetup();
void wifiPrintStatus();
bool wifiIsConnected();
WiFiClient &wifiGetInstance();
void wifiLoop();
void wifiScan();

#endif