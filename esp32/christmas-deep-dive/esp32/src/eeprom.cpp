#include "settings.h"

#include "eeprom.h"
#include <Arduino.h>
#include <EEPROM.h>

#define EEPROM_MAGIC 0xDEADBABB

struct eepromDataT eepromData;

void eepromWrite()
{
    Serial.println("EEPROM write...");
    EEPROM.put(0, eepromData);
    EEPROM.commit();
}

void eepromRead()
{
    EEPROM.get(0, eepromData);
}

void eepromSetup()
{
    EEPROM.begin(sizeof(eepromData));
    eepromRead();

    if (eepromData.magic != EEPROM_MAGIC)
    {
        Serial.println("EEPROM not initialized!");
        memset(&eepromData, 0, sizeof(eepromData));
        eepromData.magic = EEPROM_MAGIC;
        strcpy(eepromData.valueSwitch, "OFF");
        eepromWrite();
    }
    else
    {
        Serial.println("EEPROM is initialized");
    }
}

bool eepromSsidConfigured()
{
    return strlen(eepromData.ssidName) && strlen(eepromData.ssidPassword);
}

bool eepromMqttConfigured()
{
    return strlen(eepromData.mqttServer) && strlen(eepromData.mqttUser) && strlen(eepromData.mqttPassword);
}
