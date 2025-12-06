#include <Arduino.h>
#include <time.h>

#ifndef CONFIG_LOG_QUEUE_SIZE
#define CONFIG_LOG_QUEUE_SIZE 10
#endif

#ifndef CONFIG_LOG_MSG_MAX_LEN
#define CONFIG_LOG_MSG_MAX_LEN 200
#endif

#define TIME_FMT_YMD_HMS "[%Y-%m-%d %H:%M:%S] "

QueueHandle_t logQueue;

static void logTask(void *pvParameters)
{
    char logMessage[CONFIG_LOG_MSG_MAX_LEN];

    while (1)
    {
        if (xQueueReceive(logQueue, &logMessage, portMAX_DELAY) == pdTRUE)
        {
            Serial.println(logMessage);
        }
    }
}

void logMessage(const char *format, ...)
{
    char buffer[CONFIG_LOG_MSG_MAX_LEN];
    va_list args;
    va_start(args, format);

    time_t rawtime;
    time(&rawtime);
    size_t len = strftime(buffer, sizeof(buffer), TIME_FMT_YMD_HMS, gmtime(&rawtime));
    vsnprintf(buffer + len, sizeof(buffer) - len, format, args);
    va_end(args);

    if (xQueueSend(logQueue, &buffer, portMAX_DELAY) != pdTRUE)
    {
        Serial.println("Log queue full! Message dropped.");
    }
}

void logSetup(unsigned long baudRate)
{
    Serial.begin(baudRate);

    logQueue = xQueueCreate(CONFIG_LOG_QUEUE_SIZE, CONFIG_LOG_MSG_MAX_LEN);

    if (logQueue == NULL)
    {
        Serial.println("Failed to create log queue!");
        return;
    }

    xTaskCreate(logTask, "LogTask", 2048, NULL, 1, NULL);

    logMessage("Log subystem started ...");
}

String getFormattedTime(void)
{
    char buffer[256] = {0};
    time_t rawtime;
    String string;

    time(&rawtime);
    strftime(buffer, sizeof(buffer), TIME_FMT_YMD_HMS, gmtime(&rawtime));
    string = buffer;

    return string;
}