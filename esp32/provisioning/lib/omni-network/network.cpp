#include <LittleFS.h>

#include "log.h"
#include "network.h"
#include "sdkconfig.h"
#include "telnet.h"
#include "wifi.h"
#include "webserver.h"
#include "ble.h"
#include "ntp.h"
#include "modbus-tcp.h"

void networkSetup(networkSetup_t *p_network_setup)
{
    logMessage("Network subystem intializing ...");

    if (!LittleFS.begin())
    {
        logMessage("LittleFS Mount Failed");
    }

    wifiSetup(p_network_setup);
    telnetSetup();
    webserverSetup();
    bleSetup();
    ntpSetup();
    modbusTcpSetup();

    logMessage("Network subystem started ...");
}

void networkLoop()
{
    // do nothing for now
}