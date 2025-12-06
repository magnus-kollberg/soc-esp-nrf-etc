#include "settings.h"

#include "eeprom.h"
#include "sensorBme280.h"
#include "tools.h"
#include <Arduino.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <WebServer.h>

// DNS server
const byte DNS_PORT = 53;
DNSServer dnsServer;
IPAddress dsnIpAddress = toIpFromString(AP_IP_ADDRESS);

// Web server
WebServer webServer(80);

boolean captivePortal()
{
    if (!isIp(webServer.hostHeader()) && webServer.hostHeader() != (String(eepromData.ssidPassword) + ".local"))
    {
        Serial.println("Request redirected to captive portal");
        webServer.sendHeader("Location", String("http://") + toStringFromIp(webServer.client().localIP()), true);
        webServer.send(302, "text/plain", "");
        webServer.client().stop();
        return true;
    }
    return false;
}

void httpHandlePage()
{
    if (captivePortal())
    {
        return;
    }

    webServer.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    webServer.sendHeader("Pragma", "no-cache");
    webServer.sendHeader("Expires", "-1");

    String Page;
    Page += F("<!DOCTYPE html><html lang='en'><head>"
              "<meta name='viewport' content='width=device-width'>"
              "<title>CaptivePortal</title></head><body>"
              "<h1>Device config</h1>");
    if (webServer.client().localIP() == dsnIpAddress)
    {
        Page += String(F("<p>You are connected through the AP: ")) + AP_WIFI_SSID + F("</p>");
    }
    else
    {
        Page += String(F("<p>You are connected through the wifi network: ")) + eepromData.ssidName + F("</p>");
    }

    // BME820 sensor
#if SENSOR_BME280
    Page += String(F("\r\n<br />"
                     "<table><tr><th align='left'>BME280 sensor</th></tr>"
                     "<tr><td>Temperature: ")) +
            String(valueTemperature, 2) +
            F("</td></tr>"
              "<tr><td>Humidity: ") +
            String(valueHumidity, 2) +
            F("</td></tr>"
              "<tr><td>Pressure: ") +
            String(valuePressure, 2) +
            F("</td></tr>"
              "</table>");
#endif

    // AP config
    Page += String(F("\r\n<br />"
                     "<table><tr><th align='left'>AP config</th></tr>"
                     "<tr><td>SSID \"")) +
            String(AP_WIFI_SSID) +
            F("\"</td></tr>"
              "<tr><td>IP ") +
            toStringFromIp(WiFi.softAPIP()) +
            F("</td></tr>"
              "</table>"
              "\r\n<br />"
              "<table><tr><th align='left'>WLAN config</th></tr>"
              "<tr><td>SSID \"") +
            String(eepromData.ssidName) +
            F("\"</td></tr>"
              "<tr><td>IP ") +
            toStringFromIp(WiFi.localIP()) +
            F("</td></tr>"
              "</table>");

    // WLAN List
    Page += F("\r\n<br />"
              "<table><tr><th align='left'>WLAN list (refresh if any "
              "missing)</th></tr>");
    Serial.println("WiFi scan start");
    int n = WiFi.scanNetworks();
    Serial.println("WiFo scan done");
    if (n > 0)
    {
        for (int i = 0; i < n; i++)
        {
            Serial.println("SSID \"" + WiFi.SSID(i) + "\"");
            Page += String(F("\r\n<tr><td>SSID \"")) + WiFi.SSID(i) + F("\" (") + WiFi.RSSI(i) + F(")</td></tr>");
        }
    }
    else
    {
        Page += F("<tr><td>No WLAN found</td></tr>");
    }
    Page += F("</table>");

    // Config
    Page += String(F("\r\n<br /><form method='POST' "
                     "action='wifisave'><h4>Configuration:</h4>")) +
            F("<input type='text' placeholder='wlan-ssid' name='ssidName' value='") + String(eepromData.ssidName) + F("' />") +
            F("<br /><input type='password' placeholder='wlan-password' "
              "name='ssidPassword' value='") +
            String(eepromData.ssidPassword) + F("' />") +
            F("<br /><input type='text' placeholder='mqtt-server' name='mqttServer' "
              "value='") +
            String(eepromData.mqttServer) + F("' />") +
            F("<br /><input type='text' placeholder='mqtt-user' name='mqttUser' "
              "value='") +
            String(eepromData.mqttUser) + F("' />") +
            F("<br /><input type='password' placeholder='mqtt-password' "
              "name='mqttPassword' value='") +
            String(eepromData.mqttPassword) + F("' />") +
            F("<br /><input type='submit' value='Connect/Disconnect'/></form>"
              "<p><a href='/'>Refresh</a> this page.</p>"
              "</body></html>");
    webServer.send(200, "text/html", Page);
    webServer.client().stop();
}

void httpHandleSaveConfig()
{
    Serial.println("Save config");
    webServer.arg("ssidName").toCharArray(eepromData.ssidName, sizeof(eepromData.ssidName) - 1);
    webServer.arg("ssidPassword").toCharArray(eepromData.ssidPassword, sizeof(eepromData.ssidPassword) - 1);
    webServer.arg("mqttServer").toCharArray(eepromData.mqttServer, sizeof(eepromData.mqttServer) - 1);
    webServer.arg("mqttUser").toCharArray(eepromData.mqttUser, sizeof(eepromData.mqttUser) - 1);
    webServer.arg("mqttPassword").toCharArray(eepromData.mqttPassword, sizeof(eepromData.mqttPassword) - 1);
    webServer.sendHeader("Location", "/", true);
    webServer.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    webServer.sendHeader("Pragma", "no-cache");
    webServer.sendHeader("Expires", "-1");
    webServer.send(302, "text/plain", "");
    webServer.client().stop();

    Serial.println("SSID name \"" + String(eepromData.ssidName) + "\" password \"" + String(eepromData.ssidPassword) + "\"");
    Serial.println("MQTT server \"" + String(eepromData.mqttServer) + "\" user \"" + String(eepromData.mqttUser) + "\" password \"" +
                   String(eepromData.mqttPassword) + "\"");

    eepromWrite();
}

void httpHandleNotFoud()
{
    if (captivePortal())
    {
        return;
    }
    String message = F("File Not Found\n\n");
    message += F("URI: ");
    message += webServer.uri();
    message += F("\nMethod: ");
    message += (webServer.method() == HTTP_GET) ? "GET" : "POST";
    message += F("\nArguments: ");
    message += webServer.args();
    message += F("\n");

    for (uint8_t i = 0; i < webServer.args(); i++)
    {
        message += String(F(" ")) + webServer.argName(i) + F(": ") + webServer.arg(i) + F("\n");
    }
    webServer.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    webServer.sendHeader("Pragma", "no-cache");
    webServer.sendHeader("Expires", "-1");
    webServer.send(404, "text/plain", message);
}

void httpSetup()
{
    Serial.println("DNS IP address " + dsnIpAddress.toString() + " port " + DNS_PORT);

    /* Setup the DNS webServer redirecting all the domains to the dsnIpAddress */
    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer.start(DNS_PORT, "*", dsnIpAddress);
    Serial.println("DNS server started!");

    /* Setup web pages: root, wifi config pages, SO captive portal detectors and
     * not found. */
    webServer.on("/", httpHandlePage);
    webServer.on("/wifisave", httpHandleSaveConfig);
    webServer.onNotFound(httpHandleNotFoud);
    webServer.begin();
    Serial.println("Web server started!");
}

void httpLoop()
{
    dnsServer.processNextRequest();
    webServer.handleClient();
}
