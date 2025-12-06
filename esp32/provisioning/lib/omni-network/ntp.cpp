#include <NTPClient.h>
#include <WiFi.h>
#include <WiFiUdp.h>

#include "log.h"
#include "wifi.h"

#define NTP_CLIENT_WAIT_MS  1000
#define STACK_SIZE_NTP_TASK 2048

WiFiUDP ntpUDP;
NTPClient ntpClient(ntpUDP);

static void ntpClientTask(void *pvParameters)
{
    logMessage("Starting '%s' task ...", __func__);

    while (1)
    {
        if (IS_WIFI_CONNECTED)
        {
            if (ntpClient.update())
            {
                struct timeval now;
                now.tv_sec  = ntpClient.getEpochTime();
                now.tv_usec = 0;
                settimeofday(&now, NULL);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(NTP_CLIENT_WAIT_MS));
    }
}

int ntpSetup(void)
{
    ntpClient.begin();
    xTaskCreate(ntpClientTask, "ntpClientTask", STACK_SIZE_NTP_TASK, NULL, 1, NULL);

    return 0;
}
