#include "settings.h"

#include "eeprom.h"
#include "leds.h"
#include "sensorBme280.h"
#include "tools.h"
#include "wifi.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <Ticker.h>
#include "leds.h"

PubSubClient mqttClient(wifiGetInstance());

static volatile unsigned long lastConnectTry = 0;

String deviceId                              = DEVICE_ID;
String serialId                              = getSerialId();
String uniqueId                              = getUniqueId();

String lightValueTopic                       = "/semcon/iot/light/value";
String availabilityTopic                     = "semcon/" + uniqueId + "/availability";
String changeSendingListTopic                = "/semcon/iot/change_sending_list";
String changelightSendingListTopic           = "/semcon/iot/light/sendlist/";
String magnusTopic                           = changelightSendingListTopic + "Magnus";
String christianTopic                        = changelightSendingListTopic + "Christian";
String listTopic                             = changelightSendingListTopic + "#";
String changeBpmTopic                        = "/semcon/iot/change_bpm";

String List                                  = "magnus kollberg";
Ticker tickerMqtt;

static volatile int beat = 0;

void mqttBeat()
{
    if (beat > 0)
    {
        beat--;

        myPrintf("beat %d", beat);

        ledsSetBrightness(beat % 2 ? 255 : 0);
    }
    else if (beat == 0)
    {
        ledsSetBrightness(255);
        beat--;
    }
}

void mqttCallback(char *p_topic, byte *p_payload, unsigned int length)
{
    myPrintf("Received topic: '%s'", p_topic);

    if (String(p_topic) == lightValueTopic)
    {
        if (length > 3)
        {
            myPrintf("Error, length to big");
        }
        else
        {
            char value[10] = {0};
            memcpy(value, p_payload, length);
            int integer = atoi(value);
            myPrintf("value %d", integer);
            ledsSet(integer);
        }
    }
    else if (String(p_topic) == magnusTopic)
    {
        beat = 6;
        // mqttClient.publish(changeBpmTopic.c_str(), "60", true);
    }
    else if (String(p_topic) == christianTopic)
    {
        beat = 3;
        // mqttClient.publish(changeBpmTopic.c_str(), "60", true);
    }
    else if (String(p_topic) == listTopic)
    {
        // mqttClient.publish(changeBpmTopic.c_str(), "30", true);
    }
    else
    {
        // myPrintf("Error, unknown topic: '%s' payload: ", p_topic);
        // for (unsigned int i = 0; i < length; i++)
        // {
        //     Serial.print((char)p_payload[i]);
        // }
        // Serial.println("\"");
    }
}

void mqttReconnect()
{
    if (!mqttClient.connected() && (lastConnectTry == 0 || millis() > (lastConnectTry + 6000)))
    {
        mqttClient.setServer(eepromData.mqttServer, MQTT_PORT);

        Serial.print("Attempting MQTT connection to " + String(eepromData.mqttServer) + " user " + String(eepromData.mqttUser) +
                     " password " + String(eepromData.mqttPassword) + " ... ");

        // Attempt to connect
        if (mqttClient.connect(MQTT_NAME, eepromData.mqttUser, eepromData.mqttPassword, availabilityTopic.c_str(), 0, true, "offline"))
        {
            myPrintf("connected");

            // Once connected, subscribe
            mqttClient.subscribe(lightValueTopic.c_str());
            // mqttClient.publish(changeSendingListTopic.c_str(), List.c_str(), true);
            mqttClient.subscribe(magnusTopic.c_str());
            mqttClient.subscribe(listTopic.c_str());
            mqttClient.subscribe(christianTopic.c_str());
        }
        else
        {
            Serial.print("failed, rc=");
            Serial.print(mqttClient.state());
        }

        lastConnectTry = millis();
    }
}

void mqttSetup()
{
    mqttClient.setCallback(mqttCallback);
    tickerMqtt.attach(0.500, mqttBeat);
}

void mqttLoop()
{
    if (wifiIsConnected() && eepromMqttConfigured() && !mqttClient.connected())
    {
        mqttReconnect();
    }

    mqttClient.loop();
}
