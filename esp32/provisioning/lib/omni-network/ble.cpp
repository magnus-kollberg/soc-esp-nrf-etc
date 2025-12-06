#include <WiFi.h>
#include <NimBLEDevice.h>

#include "ble.h"
#include "log.h"
#include "wifi.h"

#define SERVICE_UUID                "021A9004-0382-4AEA-BFF4-6B3F1C5ADFB4"
#define WIFI_SSID_UUID              "021AFF50-0382-4AEA-BFF4-6B3F1C5ADFB4"
#define WIFI_PASSWORD_UUID          "021AFF51-0382-4AEA-BFF4-6B3F1C5ADFB4"
#define STATUS_UUID                 "021AFF52-0382-4AEA-BFF4-6B3F1C5ADFB4"
#define WIFI_NETWORKS_UUID          "021AFF53-0382-4AEA-BFF4-6B3F1C5ADFB4"

#define ARRAY_SIZE(_array)          (sizeof(_array) / sizeof(_array[0]))

#define STACK_SIZE_BLE_CONNECT_TASK 2048
#define BLE_NOTIFICATION_WAIT_MS    (1000)

typedef enum
{
    UUID_WIFI_SSID,
    UUID_WIFI_PASSWORD,
    UUID_WIFI_NETWORKS,
    UUID_STATUS,
    UUID_NOT_FOUND,
    NBR_OF_UUIDS = UUID_NOT_FOUND
} t_uuid;

typedef struct
{
    const char *p_uuid;
    NimBLECharacteristic *p_char;
    uint32_t properties;
    t_uuid uuid;
    bool subscribed;
} t_ble_char;

typedef struct
{
    NimBLEServer *p_server;
    NimBLEService *p_service;
    NimBLEAdvertising *p_advertising;
    t_ble_char chars[NBR_OF_UUIDS];
    WiFiConfig config;
    bool connected;
} t_ble;

static t_ble g_ble = {
    .p_server  = NULL,
    .p_service = NULL,
    .chars =
        {
            {.p_uuid = WIFI_SSID_UUID, .p_char = NULL, .properties = NIMBLE_PROPERTY::WRITE, .subscribed = false},
            {.p_uuid = WIFI_PASSWORD_UUID, .p_char = NULL, .properties = NIMBLE_PROPERTY::WRITE, .subscribed = false},
            {.p_uuid = WIFI_NETWORKS_UUID, .p_char = NULL, .properties = NIMBLE_PROPERTY::NOTIFY, .subscribed = false},
            {.p_uuid = STATUS_UUID, .p_char = NULL, .properties = NIMBLE_PROPERTY::NOTIFY, .subscribed = false},
        },
    .connected = false,
};

t_uuid bleGetChar(NimBLECharacteristic *p_char)
{
    const char *p_uuid = p_char->getUUID().toString().c_str();

    if (!strcasecmp(p_uuid, WIFI_SSID_UUID))
    {
        return UUID_WIFI_SSID;
    }
    else if (!strcasecmp(p_uuid, WIFI_PASSWORD_UUID))
    {
        return UUID_WIFI_PASSWORD;
    }
    else if (!strcasecmp(p_uuid, WIFI_NETWORKS_UUID))
    {
        return UUID_WIFI_NETWORKS;
    }
    else if (!strcasecmp(p_uuid, STATUS_UUID))
    {
        return UUID_STATUS;
    }

    return UUID_NOT_FOUND;
}

class ble_service_callbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer *p_server, NimBLEConnInfo &connInfo) override
    {
        logMessage("Client connect - address %s", connInfo.getAddress().toString().c_str());
    }

    void onDisconnect(NimBLEServer *p_server, NimBLEConnInfo &connInfo, int reason) override
    {
        logMessage("Client disconnected - start advertising");

        for (int i = 0; i < ARRAY_SIZE(g_ble.chars); i++)
        {
            g_ble.chars[i].subscribed = false;
        }

        g_ble.connected = false;

        NimBLEDevice::startAdvertising();
    }

} ble_service_callbacks;

class CharacteristicCallbacks : public NimBLECharacteristicCallbacks {
    void onRead(NimBLECharacteristic *p_char, NimBLEConnInfo &connInfo) override
    {
        logMessage("%s() %s", p_char->getUUID().toString().c_str());
    }

    void onWrite(NimBLECharacteristic *p_char, NimBLEConnInfo &connInfo) override
    {
        logMessage("%s() %s : %s", __func__, p_char->getUUID().toString().c_str(), p_char->getValue().c_str());

        t_uuid uuid = bleGetChar(p_char);

        logMessage("write to %d", uuid);

        switch (uuid)
        {
            case UUID_WIFI_SSID:
                g_ble.config.ssid = String(p_char->getValue().c_str());
                break;

            case UUID_WIFI_PASSWORD:
                g_ble.config.password = String(p_char->getValue().c_str());
                wifiConfigSave(&g_ble.config);
                ESP.restart();
                break;

            default:
                break;
        }
    }

    void onSubscribe(NimBLECharacteristic *p_char, NimBLEConnInfo &connInfo, uint16_t subValue) override
    {
        logMessage("Client subscribed to notifications on '%s", p_char->getUUID().toString().c_str());

        t_uuid uuid = bleGetChar(p_char);

        if (uuid != UUID_NOT_FOUND)
        {
            logMessage("subsribed to %d", uuid);
            g_ble.chars[uuid].subscribed = true;
        }
    }

} ble_char_callbacks;

void bleHandleNotificationsTask(void *pvParameters)
{
    logMessage("Starting '%s' task ...", __func__);

    String scanList     = "";
    String scanList_tmp = "";

    String status       = "";
    String status_tmp   = "";

    while (1)
    {
        uint8_t nbrOfConnectedClients = g_ble.p_server->getConnectedCount();
        if (nbrOfConnectedClients)
        {
            if (!g_ble.connected)
            {
                g_ble.connected = true;
                scanList = status = "";
            }

            if (g_ble.chars[UUID_WIFI_NETWORKS].subscribed)
            {
                scanList_tmp = wifiGetScanList();

                if (scanList_tmp != scanList)
                {
                    scanList = scanList_tmp;
                    logMessage("Sending scan list notification '%s", scanList.c_str());
                    g_ble.chars[UUID_WIFI_NETWORKS].p_char->setValue(scanList.c_str());
                    g_ble.chars[UUID_WIFI_NETWORKS].p_char->notify();
                }
            }

            if (g_ble.chars[UUID_STATUS].subscribed)
            {
                status_tmp = wifiGetStatus();

                if (status_tmp != status)
                {
                    status = status_tmp;
                    logMessage("Sending status notification '%s", status.c_str());
                    g_ble.chars[UUID_STATUS].p_char->setValue(status.c_str());
                    g_ble.chars[UUID_STATUS].p_char->notify();
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(BLE_NOTIFICATION_WAIT_MS));
    }
}

int bleSetup(void)
{
    logMessage("Starting NimBLE Server");

    NimBLEDevice::init(WiFi.getHostname());
    NimBLEDevice::setSecurityAuth(/*BLE_SM_PAIR_AUTHREQ_BOND | BLE_SM_PAIR_AUTHREQ_MITM |*/ BLE_SM_PAIR_AUTHREQ_SC);

    g_ble.p_server = NimBLEDevice::createServer();
    g_ble.p_server->setCallbacks(&ble_service_callbacks);

    g_ble.p_service = g_ble.p_server->createService(SERVICE_UUID);

    for (int i = 0; i < ARRAY_SIZE(g_ble.chars); i++)
    {
        g_ble.chars[i].p_char = g_ble.p_service->createCharacteristic(g_ble.chars[i].p_uuid, g_ble.chars[i].properties);
        g_ble.chars[i].p_char->setCallbacks(&ble_char_callbacks);
    }

    g_ble.p_service->start();

    g_ble.p_advertising = NimBLEDevice::getAdvertising();
    g_ble.p_advertising->setName(WiFi.getHostname());
    g_ble.p_advertising->addServiceUUID(g_ble.p_service->getUUID());

    g_ble.p_advertising->enableScanResponse(true);
    g_ble.p_advertising->start();

    xTaskCreate(bleHandleNotificationsTask, "bleHandleNotificationsTask", STACK_SIZE_BLE_CONNECT_TASK, NULL, 1, NULL);

    logMessage("Advertising Started");

    return 0;
}
