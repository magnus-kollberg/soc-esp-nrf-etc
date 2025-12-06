#include <WiFi.h>
#include <ESPmDNS.h>
#include <LittleFS.h>

#include <freertos/semphr.h>

#include "wifi.h"
#include "log.h"
#include "network.h"

#define STACK_SIZE_WIFI_CONNECT_TASK 2048
#define STACK_SIZE_WIFI_SCAN_TASK    2048
#define WIFI_CONNECT_WAIT_MS         5000

#define HTTP_PORT                    80
#define MQTT_PORT                    1883
struct Wifi
{
    struct config
    {
        WiFiConfig wifi_ap;
        WiFiConfig wifi_sta;
    } config;
    SemaphoreHandle_t mutexNetwork;
    SemaphoreHandle_t mutexScanNow;
    String scanList;
};

struct Wifi g_wifi = {.mutexNetwork = NULL, .mutexScanNow = NULL};

WiFiConfig wifiConfigLoad(void)
{
    WiFiConfig config;

    File file = LittleFS.open("/wifi_config.txt", "r");
    if (!file)
    {
        logMessage("No stored Wi-Fi configuration.");
        return config;
    }

    while (file.available())
    {
        String line = file.readStringUntil('\n');
        line.trim();
        if (line.startsWith("ssid="))
            config.ssid = line.substring(5);
        else if (line.startsWith("password="))
            config.password = line.substring(9);
    }

    file.close();

    logMessage("ssid: '%s'", config.ssid.c_str());
#if ENABLED(SHOW_PASSWORD)
    logMessage("password: '%s'", config.password.c_str());
#else
    logMessage("password: '********'");
#endif

    logMessage("Wi-Fi settings loaded!");

    return config;
}

int wifiConfigSave(const WiFiConfig *p_config)
{
    File file = LittleFS.open("/wifi_config.txt", "w");
    if (!file)
    {
        logMessage("Failed to save Wi-Fi p_config.");
        return -1;
    }

    file.printf("ssid=%s\npassword=%s\n", p_config->ssid.c_str(), p_config->password.c_str());
    file.close();

    logMessage("Wi-Fi settings saved!");

    return 0;
}

static void wifiConnectionStatus()
{
    if (IS_WIFI_CONNECTED)
    {
        logMessage("------------------------------------------------");
        logMessage("Wi-Fi connection status:");
        logMessage("SSID: %s", WiFi.SSID().c_str());
        logMessage("IP:   %s", WiFi.localIP().toString().c_str());
        logMessage("RSSI: %d dBm", WiFi.RSSI());
        logMessage("------------------------------------------------");
    }
    else
    {
        logMessage("Wi-Fi connection status: NOT CONNECTED");
    }
}

static void wifiScanResult(bool alwaysPrint)
{
    static bool connected = false;
    int rc                = WiFi.scanComplete();

    if (rc > 0)
    {
        if (!connected || alwaysPrint)
        {
            logMessage("------------------------------------------------");
            logMessage("Wi-Fi scan result:");
            for (int i = 0; i < rc; i++)
            {
                String ssid;
                uint8_t encryptionType;
                int32_t rssi;
                uint8_t *bssid;
                int32_t channel;
                bool isHidden;
                WiFi.getNetworkInfo(i, ssid, encryptionType, rssi, bssid, channel);

                logMessage("SSID: %-32s RSSI: %d dBm", ssid.c_str(), rssi);

                if (i)
                {
                    g_wifi.scanList = g_wifi.scanList + "," + ssid;
                }
                else
                {
                    g_wifi.scanList = ssid;
                }
            }
            logMessage("------------------------------------------------");
        }
        connected = IS_WIFI_CONNECTED;
    }
    else if (rc == WIFI_SCAN_FAILED)
    {
        logMessage("Wi-Fi scan failed!");
    }
    else if (rc == WIFI_SCAN_RUNNING)
    {
        logMessage("Wi-Fi scan running ...");
    }

    if (connected)
    {
        connected = IS_WIFI_CONNECTED;
    }
}

static void wifiConnectTask(void *pvParameters)
{
    static bool wifiConnected = false;

    logMessage("Starting '%s' task ...", __func__);

#if 1
    logMessage("Wi-Fi scan started ...");
    WiFi.scanNetworks(false, false);
    logMessage("Wi-Fi scan done...");
#else
    xSemaphoreGive(g_network.mutexScanNow);
#endif

    while (1)
    {
        if (xSemaphoreTake(g_wifi.mutexNetwork, 0))
        {
            if (!IS_WIFI_CONNECTED)
            {
                if (wifiConnected)
                {
                    wifiConnected = false;
                    logMessage("Wi-Fi connection lost to '%s'!", g_wifi.config.wifi_sta.ssid);
                    WiFi.disconnect();
                }

                logMessage("Wi-Fi reconnect to '%s' ...", g_wifi.config.wifi_sta.ssid);
                WiFi.begin(g_wifi.config.wifi_sta.ssid, g_wifi.config.wifi_sta.password);
            }

            wifiScanResult(false);

            if (IS_WIFI_CONNECTED)
            {
                if (!wifiConnected)
                {
                    wifiConnected = true;
                    logMessage("Wi-Fi succefully connected to '%s' with hostname '%s'", g_wifi.config.wifi_sta.ssid, WiFi.getHostname());
                    wifiConnectionStatus();

                    logMessage("Start mDNS/bonjour service");
                    if (MDNS.begin(WiFi.getHostname()))
                    {
                        MDNS.addService("http", "tcp", HTTP_PORT);
                        MDNS.addService("mqtt", "tcp", MQTT_PORT);
                    }
                    else
                    {
                        logMessage("Error, failed to start mDSN");
                    }
                }
            }

            xSemaphoreGive(g_wifi.mutexNetwork);
        }

        vTaskDelay(pdMS_TO_TICKS(WIFI_CONNECT_WAIT_MS)); // Check every 5 seconds
    }
}

static void wifiScanTask(void *pvParameters)
{
    logMessage("Starting '%s' task ...", __func__);

    while (1)
    {
        if (xSemaphoreTake(g_wifi.mutexScanNow, portMAX_DELAY))
        {
            if (xSemaphoreTake(g_wifi.mutexNetwork, portMAX_DELAY))
            {
                logMessage("Wi-Fi scan started ...");
                int attempts = 3;
                while (attempts-- > 0)
                {
                    int result = WiFi.scanNetworks();
                    if (WiFi.scanNetworks() != WIFI_SCAN_FAILED)
                    {
                        break;
                    }

                    logMessage("Scan failed, retrying ...");
                    WiFi.disconnect();
                    vTaskDelay(pdMS_TO_TICKS(500));
                }

                logMessage("Wi-Fi scan done ...");

                wifiScanResult(true);

                xSemaphoreGive(g_wifi.mutexNetwork);
            }
        }
    }
}

static void wifiOnEvent(arduino_event_t *sys_event)
{
    switch (sys_event->event_id)
    {
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            logMessage("Connected IP address : '%s'", IPAddress(sys_event->event_info.got_ip.ip_info.ip.addr).toString().c_str());
            break;
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            // logMessage("Disconnected. Connecting to the AP again... ");
            break;
        case ARDUINO_EVENT_PROV_START:
            logMessage("Provisioning started. Give Credentials of your access point using smartphone app");
            break;
        case ARDUINO_EVENT_PROV_CRED_RECV:
        {
            logMessage("Received Wi-Fi credentials");
            logMessage("SSID : '%s'", (const char *)sys_event->event_info.prov_cred_recv.ssid);
#if ENABLED(SHOW_PASSWORD)
            logMessage("Password : '%s'", (char const *)sys_event->event_info.prov_cred_recv.password);
#else
            logMessage("Password : '********'");
#endif
            break;
        }
        case ARDUINO_EVENT_PROV_CRED_FAIL:
        {
            logMessage("Provisioning failed! Please reset to factory and retry provisioning");
            if (sys_event->event_info.prov_fail_reason == WIFI_PROV_STA_AUTH_ERROR)
            {
                logMessage("Wi-Fi AP password incorrect");
            }
            else
            {
                logMessage("Wi-Fi AP not found.... Add API \" nvs_flash_erase() \" before beginProvision()");
            }
            break;
        }
        case ARDUINO_EVENT_PROV_CRED_SUCCESS:
            logMessage("Provisioning Successful");
            break;
        case ARDUINO_EVENT_PROV_END:
            logMessage("Provisioning Ends");
            break;
        default:
            break;
    }
}

void wifiScanNow(void)
{
    ENTER();
    xSemaphoreGive(g_wifi.mutexScanNow);
    EXIT();
}

String wifiGetScanList(void)
{
    return g_wifi.scanList;
}

String wifiGetStatus(void)
{
    String status;

    if (IS_WIFI_CONNECTED)
    {
        status = "Connected " + WiFi.SSID() + " " + WiFi.RSSI();
    }
    else
    {
        status = "Not connected";
    }

    return status;
}

void wifiConnect()
{
}

int wifiSetup(networkSetup_t *p_network_setup)
{
    logMessage("Wi-Fi subsystem intializing ...");

    g_wifi.config.wifi_sta         = wifiConfigLoad();
    g_wifi.config.wifi_ap.ssid     = p_network_setup->apName;
    g_wifi.config.wifi_ap.password = "";

    WiFi.setHostname(g_wifi.config.wifi_ap.ssid.c_str());
    logMessage("Hostname '%s'", WiFi.getHostname());

    g_wifi.mutexScanNow = xSemaphoreCreateCounting(1, 0);
    if (g_wifi.mutexScanNow == NULL)
    {
        logMessage("Failed to create scan mutex");
        return -1;
    }

    g_wifi.mutexNetwork = xSemaphoreCreateMutex();
    if (g_wifi.mutexNetwork == NULL)
    {
        logMessage("Failed to create network mutex");
        return -1;
    }

    logMessage("Start provisioning of '%s' ...", g_wifi.config.wifi_ap.ssid.c_str());
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(g_wifi.config.wifi_ap.ssid, p_network_setup->apPassprase);

    xTaskCreate(wifiConnectTask, "wifiConnectTask", STACK_SIZE_WIFI_CONNECT_TASK, NULL, 1, NULL);
    xTaskCreate(wifiScanTask, "wifiScanTask", STACK_SIZE_WIFI_SCAN_TASK, NULL, 1, NULL);

    WiFi.onEvent(wifiOnEvent);

    return 0;
}