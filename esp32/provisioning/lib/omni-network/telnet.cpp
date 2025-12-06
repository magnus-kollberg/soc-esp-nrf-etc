#include <WiFi.h>
#include <LittleFS.h>
#include <freertos/FreeRTOS.h>

#include "log.h"
#include "wifi.h"
#include "telnet.h"

#define STACK_SIZE_TELNET_TASK        4096
#define TELNET_SERVER_WAIT_MS         1000
#define TELNET_CONNECTION_TIMEOUT_SEC 60
#define TELNET_PORT                   23
#define CR                            0x0d

typedef void (*telnetCommandHandler_t)(WiFiClient *client, const char *textBuff);

typedef struct
{
    const char *command;
    telnetCommandHandler_t handler;
    const char *help;
} telnetCommand_t;

struct Telnet
{
    struct config
    {
        TelnetConfig telnet;
    } config;
};

struct Telnet g_telnet;

WiFiServer telnetServer(TELNET_PORT);

TelnetConfig telnetConfigLoad(void)
{
    TelnetConfig config;

    File file = LittleFS.open("/telnet_config.txt", "r");
    if (!file)
    {
        logMessage("No stored Telnet configuration.");
        return config;
    }

    while (file.available())
    {
        String line = file.readStringUntil('\n');
        line.trim();
        if (line.startsWith("enabled="))
            config.enabled = line.substring(8).toInt();
    }

    file.close();

    logMessage("Telnet settings loaded!");

    return config;
}

int telnetConfigSave(const TelnetConfig *p_config)
{
    File file = LittleFS.open("/telnet_config.txt", "w");
    if (!file)
    {
        logMessage("Failed to save Wi-Fi config.");
        return -1;
    }

    file.printf("enabled=%d\n", p_config->enabled);
    file.close();

    logMessage("Telnet settings saved!");

    return 0;
}

static void telnetPrintBanner(WiFiClient *client)
{
    client->println("Welcome to " + String(WiFi.getHostname()) + " Telnet Server!");
    client->println("Type '?' for help.");
    client->println("");
    client->flush();

    while (client->available())
    {
        client->read();
    }
}

static void telnetPrintPrompt(WiFiClient *client)
{
    client->flush();
    client->print("> ");
}

void listFilesInDirectory(WiFiClient *client, fs::FS &fs, const char *dirname, uint8_t levels)
{
    logMessage("Listing directory: %s", dirname);

    char logBuffer[128] = {0};

    client->println("DIR : " + String(dirname));

    File root = fs.open(dirname);
    if (!root || !root.isDirectory())
    {
        logMessage("Failed to open directory '%s' for reading", dirname);
        client->println("Failed to open directory '" + String(dirname) + "' for reading");
        return;
    }

    File file = root.openNextFile();
    while (file)
    {
        if (file.isDirectory())
        {
            logMessage("DIR : %s", file.name());
            client->println("DIR: " + String(file.name()));
            if (levels)
            {
                listFilesInDirectory(client, fs, file.name(), levels - 1);
            }
        }
        else
        {
            snprintf(logBuffer, sizeof(logBuffer) - 1, "%-30s %5d bytes", file.name(), file.size());
            logMessage(logBuffer);
            client->println(logBuffer);
        }
        file = root.openNextFile();
    }
}

static void telnetCommandStats(WiFiClient *client, const char *command)
{
    logMessage("%s(): '%s'", __func__, command);

    client->println("Date:        " + getFormattedTime());
    client->println();

    client->println("ssid:        " + WiFi.SSID());
    client->println("rssi:        " + String(WiFi.RSSI()) + " dBm");
    client->println("local_ip:    " + WiFi.localIP().toString());
    client->println();

    client->println("free_ram:    " + String(heap_caps_get_free_size(MALLOC_CAP_INTERNAL)) + " bytes");
    client->println("used_ram:    " + String(heap_caps_get_total_size(MALLOC_CAP_INTERNAL) - heap_caps_get_free_size(MALLOC_CAP_INTERNAL)) +
                    " bytes");
    client->println("total_ram:   " + String(heap_caps_get_total_size(MALLOC_CAP_INTERNAL)) + " bytes");
    client->println();

    client->println("free_flash:  " + String(LittleFS.totalBytes() - LittleFS.usedBytes()) + " bytes");
    client->println("used_flash:  " + String(LittleFS.usedBytes()) + " bytes");
    client->println("total_flash: " + String(LittleFS.totalBytes()) + " bytes");
    client->println();

    UBaseType_t taskCount = uxTaskGetNumberOfTasks();
    client->println("task_count:  " + String(taskCount));
}

static void telnetCommandDate(WiFiClient *client, const char *command)
{
    logMessage("%s(): '%s'", __func__, command);

    client->println(getFormattedTime());
}

static void telnetCommandListFiles(WiFiClient *client, const char *command)
{
    logMessage("%s(): '%s'", __func__, command);

    listFilesInDirectory(client, LittleFS, "/", 3);
}

static void telnetCommandReboot(WiFiClient *client, const char *command)
{
    logMessage("%s(): '%s'", __func__, command);

    client->println();
    client->println("Rebooting ...");
    client->stop();
    ESP.restart();
}

static void telnetCommandBye(WiFiClient *client, const char *command)
{
    logMessage("%s(): '%s'", __func__, command);

    client->println("Shutting down ...");
    client->stop();
}

static void telnetCommandHelp(WiFiClient *client, const char *command);

static telnetCommand_t telnetCommands[] = {
    {"stats", telnetCommandStats, "Print current stats"},
    {"date", telnetCommandDate, "Print current date and time"},
    {"ls", telnetCommandListFiles, "List all files"},
    {"reboot", telnetCommandReboot, "Reboot the device"},
    {"bye", telnetCommandBye, "Exit telnet session"},
    {"?", telnetCommandHelp, "Print this help message"},
};

static void telnetCommandHelp(WiFiClient *client, const char *command)
{
    char logBuffer[128] = {0};

    logMessage("%s(): '%s'", __func__, command);

    client->println("Help commands:");

    for (size_t i = 0; i < sizeof(telnetCommands) / sizeof(telnetCommand_t); i++)
    {
        snprintf(logBuffer, sizeof(logBuffer) - 1, "%-10s - %s", telnetCommands[i].command, telnetCommands[i].help);
        client->println(logBuffer);
    }
}

static void telnetClientServerTask(void *pvParameters)
{
    WiFiClient telnetClient    = 0;
    unsigned long lastActivity = 0;

    logMessage("Starting '%s' task ...", __func__);

    while (1)
    {
        if (IS_WIFI_CONNECTED)
        {
            if (telnetServer.hasClient())
            {
                if (telnetClient && telnetClient.connected())
                {
                    logMessage("Client already connected. Rejecting new connection.");
                    telnetServer.available().stop();
                }
                else
                {
                    logMessage("Telnet client connected ...");
                    telnetClient = telnetServer.available();
                    telnetPrintBanner(&telnetClient);
                    telnetPrintPrompt(&telnetClient);

                    lastActivity = millis();
                }
            }

            if (telnetClient && telnetClient.connected() && telnetClient.available())
            {
                String string = telnetClient.readStringUntil(CR);
                string.trim();
                logMessage("Received: '%s'", string.c_str());

                for (size_t i = 0; i < sizeof(telnetCommands) / sizeof(telnetCommand_t); i++)
                {
                    if (string.startsWith(telnetCommands[i].command))
                    {
                        telnetClient.println();
                        telnetCommands[i].handler(&telnetClient, string.c_str());
                        telnetClient.println();
                        break;
                    }
                }

                telnetPrintPrompt(&telnetClient);

                lastActivity = millis();
            }

            if (telnetClient && !telnetClient.connected())
            {
                logMessage("Telnet client disconnected!");
                telnetClient.stop();
                telnetClient = 0;
            }

            if (telnetClient && ((millis() - lastActivity) > (TELNET_CONNECTION_TIMEOUT_SEC * 1000)))
            {
                logMessage("Client timeout disconnect ...");
                telnetClient.println();
                telnetClient.println("Timeout disconnect ...");
                telnetClient.stop();
                telnetClient = 0;
            }
        }
        else
        {
            vTaskDelay(pdMS_TO_TICKS(TELNET_SERVER_WAIT_MS));
        }
    }
}

int telnetSetup(void)
{
    logMessage("Telnet subsystem intializing ...");

    g_telnet.config.telnet = telnetConfigLoad();

#if CONFIG_NETWORK_PROVISONING != CONFIG_NETWORK_PROVISONING_BLE
    telnetServer.begin();
    telnetServer.setNoDelay(true);
    xTaskCreate(telnetClientServerTask, "telnetClientServerTask", STACK_SIZE_TELNET_TASK, NULL, 1, NULL);
#endif

    return 0;
}
