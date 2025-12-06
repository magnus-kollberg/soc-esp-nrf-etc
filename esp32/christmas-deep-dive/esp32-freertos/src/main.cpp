#include <Arduino.h>

#include "leds.h"

// Task handles
TaskHandle_t Task2;

// Task functions
void Task1Code(void *pvParameters)
{
    for (;;)
    {
        Serial.println("Task 1 is running");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void Task2Code(void *pvParameters)
{
    for (;;)
    {
        Serial.println("Task 2 is running");
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

void setup()
{
    Serial.begin(115200);

    ledSetup();

    // Create tasks
    // xTaskCreate(Task2Code, "Task2", 1000, NULL, 1, &Task2);
}

void loop()
{
    // Do nothing; tasks are running
}