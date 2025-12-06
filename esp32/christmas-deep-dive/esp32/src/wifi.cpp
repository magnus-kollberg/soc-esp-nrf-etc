#include "settings.h"

#include "eeprom.h"
#include "http.h"
#include "tools.h"
#include <Arduino.h>
#include <WiFi.h>

WiFiClient wifiClient;
bool wifiDoConnect                      = true;
bool wifiConnected                      = false;
static unsigned long wifiLastConnectTry = 0;
static unsigned int wifiStatus          = WL_IDLE_STATUS;
#define WIFI_RECONNECT_TIME_SEC 10

IPAddress wifiIpAddress = toIpFromString(AP_IP_ADDRESS);
IPAddress wifiNetMask   = toIpFromString(AP_NET_MASK);

void wifiPrintStatus()
{
    Serial.println("SSID: " + WiFi.SSID());
    Serial.println("IP Address: " + toStringFromIp(WiFi.localIP()));
    Serial.println("signal strength (RSSI): " + String(WiFi.RSSI()) + " dBm");
}

void wifiSetup()
{
    WiFi.softAP(AP_WIFI_SSID, AP_WIFI_PASSWORD);
    Serial.println("Wait 100 ms for AP_START...");
    delay(100);
    // WiFi.softAPConfig(wifiIpAddress, wifiIpAddress, wifiNetMask);
    Serial.println("AP IP address: " + WiFi.softAPIP().toString());

    // Blink 4 times to show that we connected to WiFi.
    ledBlinkBlue(4);
}

void wifiScan()
{
    int n = WiFi.scanNetworks();
    Serial.println("WiFo scan done");
    if (n > 0)
    {
        for (int i = 0; i < n; i++)
        {
            Serial.println("SSID \"" + WiFi.SSID(i) + "\"");
        }
    }
}

WiFiClient &wifiGetInstance()
{
    return wifiClient;
}

bool wifiConnect()
{
    Serial.println("Connecting to SSID \"" + String(eepromData.ssidName) + "\" password \"" + String(eepromData.ssidPassword) + "\"");

    WiFi.disconnect();
    WiFi.begin(eepromData.ssidName, eepromData.ssidPassword);
    if (WiFi.waitForConnectResult() == WL_CONNECTED)
    {
        Serial.println("WiFi connection OK");
        wifiConnected = true;
    }
    else
    {
        Serial.println("WiFi connection FAILED");
        wifiConnected = false;
    }
    return wifiConnected;
}

bool wifiIsConnected()
{
    return wifiConnected;
}

void wifiLoop()
{
    if (!eepromSsidConfigured())
    {
        return;
    }

    if (wifiDoConnect && !wifiIsConnected())
    {
        Serial.println("WiFi connect requested");
        wifiConnect();
        wifiLastConnectTry = millis();
        wifiDoConnect      = false;
    }

    unsigned int s = WiFi.status();

    if (s != WL_CONNECTED && millis() > (wifiLastConnectTry + WIFI_RECONNECT_TIME_SEC * 1000))
    {
        Serial.println("Try new WiFi connect request");
        wifiDoConnect = true;
    }

    if (wifiStatus != s)
    {
        Serial.print("New WIFI Status: " + s);
        wifiStatus = s;
        if (s == WL_CONNECTED)
        {
            WiFi.hostname(getUniqueId());

            Serial.println("\nWiFi connected with MAC: " + WiFi.macAddress() + " IP: " + +WiFi.localIP()[0] + "." + WiFi.localIP()[1] +
                           "." + WiFi.localIP()[2] + "." + WiFi.localIP()[3]);
            wifiConnected = true;
        }
        else if (s == WL_NO_SSID_AVAIL)
        {
            WiFi.disconnect();
            wifiConnected = false;
        }
    }
}
