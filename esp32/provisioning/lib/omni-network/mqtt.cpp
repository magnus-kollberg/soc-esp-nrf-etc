#include <LittleFS.h>

#include "mqtt.h"
#include "log.h"

MQTTConfig mqttConfigLoad(void)
{
    MQTTConfig config;

    File file = LittleFS.open("/mqtt_config.txt", "r");
    if (!file)
    {
        logMessage("No stored MQTT configuration.");
        return config;
    }

    while (file.available())
    {
        String line = file.readStringUntil('\n');
        line.trim();
        if (line.startsWith("server="))
            config.server = line.substring(7);
        else if (line.startsWith("port="))
            config.port = line.substring(5).toInt();
        else if (line.startsWith("user="))
            config.user = line.substring(5);
        else if (line.startsWith("password="))
            config.password = line.substring(9);
    }

    file.close();

    logMessage("MQTT settings loaded!");

    return config;
}

int mqttConfigSave(const MQTTConfig *config)
{
    File file = LittleFS.open("/mqtt_config.txt", "w");
    if (!file)
    {
        logMessage("Failed to save MQTT config.");
        return -1;
    }

    file.printf(
        "server=%s\nport=%d\nuser=%s\npassword=%s\n", config->server.c_str(), config->port, config->user.c_str(), config->password.c_str());
    file.close();

    logMessage("MQTT settings saved!");

    return 0;
}

int mqttSetup(void)
{
    logMessage("MQTT subsystem intializing ...");

    return 0;
}
