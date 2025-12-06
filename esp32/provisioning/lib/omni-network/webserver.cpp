

#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ESPAsyncDNSServer.h>

#include "log.h"
#include "wifi.h"
#include "mqtt.h"
#include "telnet.h"

#define HTTP_PORT                      80

#define HTTP_OK                        200
#define HTTP_BAD_REQUEST               400
#define HTTP_NOT_FOUND                 404
#define DNS_PORT                       53

#define HTTP_GET_WIFI_FETCH_STATUS     "/wifi_fetch_status"
#define HTTP_GET_WIFI_SCAN_RESULT      "/wifi_scan_result"
#define HTTP_GET_WIFI_SCAN_NEW         "/wifi_scan_new"
#define HTTP_POST_WIFI_CONNECT         "/wifi_connect"
#define HTTP_GET_MQTT_FETCH_SETTINGS   "/mqtt_fetch_settings"
#define HTTP_POST_MQTT_UPDATE_SETTINGS "/mqtt_update_settings"
#define HTTP_GET_SYSTEM_FETCH_STATUS   "/system_fetch_status"
#define HTTP_POST_SET_TELNET           "/set_telnet"
#define HTTP_GET_GET_TELNET            "/get_telnet"
#define HTTP_GET_LOGO_PNG              "/logo.png"

AsyncDNSServer dnsServer;
AsyncWebServer webServer(HTTP_PORT);

class RequestHandler : public AsyncWebHandler {
  public:
#if ENABLED(JSON_DUMP)
    void dumpRequest(AsyncWebServerRequest *request)
    {
        logMessage("Method: %s", request->methodToString());
        logMessage("URL: %s", request->url().c_str());
        // Print Headers
        logMessage("Headers:");
        for (int i = 0; i < request->headers(); i++)
        {
            AsyncWebHeader *h = request->getHeader(i);
            logMessage("  %s: %s", h->name().c_str(), h->value().c_str());
        }

        // Print Query Parameters
        logMessage("Query Parameters:");
        for (int i = 0; i < request->params(); i++)
        {
            AsyncWebParameter *p = request->getParam(i);
            logMessage("  %s: %s", p->name().c_str(), p->value().c_str());
        }
    }
#endif

    void requestWiFiFetchStatus(AsyncWebServerRequest *request)
    {
        String json = "{";
        json += "\"device_id\":\"" + String(WiFi.getHostname()) + "\",";
        json += "\"ssid\":\"" + WiFi.SSID() + "\",";
        json += "\"rssi\":" + String(WiFi.RSSI()) + ",";
        json += "\"local_ip\":\"" + WiFi.localIP().toString() + "\"";
        json += "}";

#if ENABLED(JSON_DUMP)
        logMessage("%s() json: '%s'", __func__, json.c_str());
#endif
        request->send(HTTP_OK, "application/json", json);
    }

    void requestWiFiConnect(AsyncWebServerRequest *request)
    {
        ENTER();

#if ENABLED(JSON_DUMP)
        dumpRequest(request);
#endif

        if (request->hasParam("ssid", true) && request->hasParam("password", true))
        {
            WiFiConfig config;
            config.ssid     = request->getParam("ssid", true)->value();
            config.password = request->getParam("password", true)->value();

            logMessage("Received Wi-Fi credentials:");
            logMessage("SSID: %s ", config.ssid.c_str());
#if ENABLED(SHOW_PASSWORD)
            logMessage("Password: %s", config.password.c_str());
#else
            logMessage("Password: ********");
#endif
            wifiConfigSave(&config);
            request->send(HTTP_OK, "text/plain", "Wi-Fi settings saved. Reconnecting ...");
            ESP.restart();
        }
        else
        {
            logMessage("Error, missing Wi-Fi parameters %d %d", request->hasParam("ssid", true), request->hasParam("password", true));
            request->send(HTTP_BAD_REQUEST, "text/plain", "Missing parameters.");
        }
        EXIT();
    }

    void requestWiFiScanResult(AsyncWebServerRequest *request)
    {
        // ENTER();
        int rc      = WiFi.scanComplete();

        String json = "[";
        for (int i = 0; i < rc; i++)
        {
            if (i)
            {
                json += ",";
            }
            json += "{\"ssid\":\"" + WiFi.SSID(i) + "\", \"rssi\":" + String(WiFi.RSSI(i)) + "}";
        }
        json += "]";

#if ENABLED(JSON_DUMP)
        logMessage("%s() json: '%s'", __func__, json.c_str());
#endif

        request->send(HTTP_OK, "application/json", json);
        // EXIT();
    }

    void requestWiFiScanNew(AsyncWebServerRequest *request)
    {
        // ENTER();
        request->send(HTTP_OK, "text/plain", "OK");
        wifiScanNow();
        // EXIT();
    }

    void requestMQTTFetchSettings(AsyncWebServerRequest *request)
    {
        MQTTConfig config = mqttConfigLoad();

        // ENTER();
        String json = "{";
        json += "\"server\":\"" + config.server + "\",";
        json += "\"port\":" + String(config.port) + ",";
        json += "\"user\":\"" + config.user + "\"";
        json += "}";

#if ENABLED(JSON_DUMP)
        logMessage("%s() json: '%s'", __func__, json.c_str());
#endif

        request->send(HTTP_OK, "application/json", json);
        // EXIT();
    }

    void requestMQTTUpdateSettings(AsyncWebServerRequest *request)
    {
        // ENTER();
        if (!request->hasParam("server", true) || !request->hasParam("port", true) || !request->hasParam("user", true) ||
            !request->hasParam("password", true))
        {
            logMessage("Error, missing MQTT parameters %d %d %d %d",
                       request->hasParam("server", true),
                       request->hasParam("port", true),
                       request->hasParam("user", true),
                       request->hasParam("password", true));
            request->send(HTTP_BAD_REQUEST, "text/plain", "Missing parameters");
            return;
        }

        MQTTConfig config;

        config.server   = request->getParam("server", true)->value();
        config.port     = request->getParam("port", true)->value().toInt();
        config.user     = request->getParam("user", true)->value();
        config.password = request->getParam("password", true)->value();

        mqttConfigSave(&config);

        request->send(HTTP_OK, "text/plain", "MQTT settings saved!");
        // EXIT();
    }

    void requestSystemFetchStatus(AsyncWebServerRequest *request)
    {
        size_t total_heap = heap_caps_get_total_size(MALLOC_CAP_INTERNAL);
        size_t free_heap  = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
        size_t used_heap  = total_heap - free_heap;

        size_t total_fs   = LittleFS.totalBytes();
        size_t used_fs    = LittleFS.usedBytes();
        size_t free_fs    = total_fs - used_fs;

        String json       = "{";
        json += "\"current_time\":\"" + getFormattedTime() + "\",";
        json += "\"free_ram\":" + String(free_heap) + ",";
        json += "\"used_ram\":" + String(used_heap) + ",";
        json += "\"total_ram\":" + String(total_heap) + ",";

        json += "\"free_flash\":" + String(free_fs) + ",";
        json += "\"used_flash\":" + String(used_fs) + ",";
        json += "\"total_flash\":" + String(total_fs) + ",";

        UBaseType_t taskCount = uxTaskGetNumberOfTasks();
        json += "\"task_count\":" + String(taskCount);

        // json += "\"tasks\":\"";
        // TaskStatus_t *taskStatusArray;
        // taskStatusArray = (TaskStatus_t *)malloc(taskCount * sizeof(TaskStatus_t));
        // if (taskStatusArray)
        // {
        //     // taskCount = uxTaskGetSystemState(taskStatusArray, taskCount, NULL);
        //     // for (UBaseType_t i = 0; i < taskCount; i++)
        //     // {
        //     //     json += String(taskStatusArray[i].pcTaskName) + "\";
        //     // }
        //     free(taskStatusArray);
        // }

        json += "}";

#if ENABLED(JSON_DUMP)
        logMessage("%s() json: '%s'", __func__, json.c_str());
#endif

        request->send(HTTP_OK, "application/json", json);
    }

    void requestSetTelnet(AsyncWebServerRequest *request)
    {
        ENTER();
#if ENABLED(JSON_DUMP)
        dumpRequest(request);
#endif

        if (request->hasParam("enabled", true))
        {
            TelnetConfig config = telnetConfigLoad();
            String param        = request->getParam("enabled", true)->value();

            logMessage("set enabled '%s'", param.c_str());

            if (config.enabled != (param == "true"))
            {
                logMessage("set enabled %d '%s'", (param == "true"), param.c_str());
                config.enabled = (param == "true");
                telnetConfigSave(&config);
            }

            request->send(HTTP_OK, "text/plain", "OK");
        }
        else
        {
            logMessage("Error, missing telnet parameter %d", request->hasParam("enabled", true));
            request->send(HTTP_BAD_REQUEST, "text/plain", "Missing parameter");
            return;
        }
    }

    void requestGetTelnet(AsyncWebServerRequest *request)
    {
        TelnetConfig config = telnetConfigLoad();
        String json         = "{\"enabled\":" + String(config.enabled ? "true" : "false") + "}";

#if ENABLED(JSON_DUMP)
        logMessage("%s() json: '%s'", __func__, json.c_str());
#endif

        request->send(HTTP_OK, "application/json", json);
    }

    RequestHandler()
    {
        webServer.on(
            HTTP_GET_WIFI_FETCH_STATUS, HTTP_GET, [this](AsyncWebServerRequest *request) { this->requestWiFiFetchStatus(request); });
        webServer.on(HTTP_GET_WIFI_SCAN_RESULT, HTTP_GET, [this](AsyncWebServerRequest *request) { this->requestWiFiScanResult(request); });
        webServer.on(HTTP_GET_WIFI_SCAN_NEW, HTTP_GET, [this](AsyncWebServerRequest *request) { this->requestWiFiScanNew(request); });
        webServer.on(HTTP_POST_WIFI_CONNECT, HTTP_POST, [this](AsyncWebServerRequest *request) { this->requestWiFiConnect(request); });
        webServer.on(
            HTTP_GET_MQTT_FETCH_SETTINGS, HTTP_GET, [this](AsyncWebServerRequest *request) { this->requestMQTTFetchSettings(request); });
        webServer.on(HTTP_POST_MQTT_UPDATE_SETTINGS, HTTP_POST, [this](AsyncWebServerRequest *request) {
            this->requestMQTTUpdateSettings(request);
        });
        webServer.on(
            HTTP_GET_SYSTEM_FETCH_STATUS, HTTP_GET, [this](AsyncWebServerRequest *request) { this->requestSystemFetchStatus(request); });
        webServer.on(HTTP_POST_SET_TELNET, HTTP_POST, [this](AsyncWebServerRequest *request) { this->requestSetTelnet(request); });
        webServer.on(HTTP_GET_GET_TELNET, HTTP_GET, [this](AsyncWebServerRequest *request) { this->requestGetTelnet(request); });
        webServer.on(HTTP_GET_LOGO_PNG, HTTP_GET, [](AsyncWebServerRequest *request) {
            request->send(LittleFS, "/knightec_200x106.png", "image/png");
        });
    }

    virtual ~RequestHandler()
    {
    }

    bool canHandle(AsyncWebServerRequest *request) override
    {
        return true;
    }

    void handleRequest(AsyncWebServerRequest *request)
    {
        AsyncWebServerResponse *response = request->beginResponse(LittleFS, "/index.html", "text/html");
        request->send(response);
    }

  private:
    String _device_id;
};

int webserverSetup(void)
{
    logMessage("WEBSERVER subsystem intializing ...");

    webServer.addHandler(new RequestHandler());
    webServer.begin();
    dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());

    return 0;
}