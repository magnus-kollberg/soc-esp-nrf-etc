#include <ModbusTCP.h>

#include "log.h"
#include "wifi.h"

#define MODBUS_TCP_CLIENT_WAIT_MS  1000
#define STACK_SIZE_MODBUS_TCP_TASK 2048
#define MODBUS_TCP_PORT            502

IPAddress remote(192, 168, 1, 247);
ModbusTCP modbus;

#define REG 0

static void modbusTcpTask(void *pvParameters)
{
    logMessage("Starting '%s' task ...", __func__);

    while (1)
    {
        if (IS_WIFI_CONNECTED)
        {
            if (!modbus.isConnected(remote))
            {
                logMessage("Not connected, trying to connect");
                modbus.connect(remote, MODBUS_TCP_PORT); // Port 502 is default
            }
            else
            {
                uint16_t result;

                logMessage("Connected...");

                modbus.task();
                vTaskDelay(pdMS_TO_TICKS(MODBUS_TCP_CLIENT_WAIT_MS * 9));

                // Read 1 holding register at address 0 (Modbus register 40001)
                uint16_t trans = modbus.readHreg(remote, REG, &result, 1);
                logMessage("Transaction id %d", trans);

                while (modbus.isTransaction(trans))
                {
                    modbus.task();
                    vTaskDelay(pdMS_TO_TICKS(10));
                }

                logMessage("Register %d = 0x%04x, rc %d", REG, result, trans);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(MODBUS_TCP_CLIENT_WAIT_MS));
    }
}

int modbusTcpSetup(void)
{
    // modbus.onRaw(cbTcpRaw);

    modbus.client();
    modbus.begin();
    xTaskCreate(modbusTcpTask, "modbusTcpTask", STACK_SIZE_MODBUS_TCP_TASK, NULL, 1, NULL);

    return 0;
}
