#include <Arduino.h>
#include <FastLED.h>

// LED strip configuration
#define LED_PIN       14      // GPIO pin connected to the LED strip's data line
#define NUM_LEDS      12      // Number of LEDs in your strip
#define LED_TYPE      WS2812B // Type of LED (e.g., WS2812, WS2812B, APA102, etc.)
#define COLOR_ORDER   GRB     // Color order (GRB for WS2812B, RGB for others)

#define TASK_SLEEP_MS 50

CRGB leds[NUM_LEDS];

/**
 * @brief Handle for the LED task.
 *
 * This handle is used to manage the LED task in the FreeRTOS environment.
 * It allows for operations such as starting, stopping, and querying the state
 * of the task.
 */
TaskHandle_t LedTask;

typedef struct
{
    int value;
    int brightness;
    TaskHandle_t task;
} led_t;

led_t m_led = {
    .value      = 20,
    .brightness = 80,
};

void ledTask(void *pvParameters)
{
    bool update = true;

    while (1)
    {
        if (update)
        {
            if (m_led.value > 128)
            {
                CRGB led = leds[NUM_LEDS - 1];

                for (int i = NUM_LEDS - 1; i > 0; i--)
                {
                    leds[i] = leds[i - 1];
                }
                leds[0] = led;

                FastLED.show();
            }
            else
            {
                CRGB led = leds[0];

                for (int i = 1; i < NUM_LEDS; i++)
                {
                    leds[i - 1] = leds[i];
                }
                leds[NUM_LEDS - 1] = led;

                FastLED.show();
            }
        }

        vTaskDelay(TASK_SLEEP_MS / portTICK_PERIOD_MS);
    }
}

void ledSetup()
{
    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

    // Example: Light up the strip with a rainbow effect
    for (int i = 0; i < (NUM_LEDS - 0); i++)
    {
        leds[i] = CHSV((i * 255) / NUM_LEDS, 255, 255); // Hue, Saturation, Value
    }

    randomSeed(analogRead(0));

    FastLED.setBrightness(m_led.brightness);
    FastLED.show();

    xTaskCreate(ledTask, "LedTask", 1000, NULL, 1, &m_led.task);
}
