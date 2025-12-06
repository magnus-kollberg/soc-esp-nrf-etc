#include "settings.h"
#include "log.h"
#include "network.h"

networkSetup_t g_networkSetup;

void setup()
{
    g_networkSetup.apName      = AP_WIFI_SSID;
    g_networkSetup.apPassprase = AP_WIFI_PASSWORD;

    logSetup(SERIAL_SPEED);
    networkSetup(&g_networkSetup);
}

void loop()
{
    networkLoop();
}