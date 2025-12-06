#ifndef __mqtt_h__
#define __mqtt_h__

#include <Arduino.h>

struct MQTTConfig
{
    String server;
    int port;
    String user;
    String password;
};

int mqttSetup(void);
int mqttConfigSave(const MQTTConfig *p_config);
MQTTConfig mqttConfigLoad(void);

#endif