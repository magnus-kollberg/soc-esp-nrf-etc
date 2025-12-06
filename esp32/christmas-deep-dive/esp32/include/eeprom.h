#ifndef __eeprom_h__
#define __eeprom_h__

#include <Arduino.h>

struct eepromDataT
{
    uint32_t magic;
    char ssidName[33];
    char ssidPassword[65];
    char mqttServer[33];
    char mqttUser[33];
    char mqttPassword[65];
    char valueSwitch[4];
};

extern struct eepromDataT eepromData;

void eepromSetup();
void eepromWrite();
void eepromRead();
bool eepromSsidConfigured();
bool eepromMqttConfigured();

#endif