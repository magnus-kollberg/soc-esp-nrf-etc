#include <FastLED.h>
#include <Ticker.h>

#include "tools.h"
#include <pins_arduino.h>

// LED strip configuration
#define LED_PIN       14      // GPIO pin connected to the LED strip's data line
#define NUM_LEDS      12      // Number of LEDs in your strip
#define LED_TYPE      WS2812B // Type of LED (e.g., WS2812, WS2812B, APA102, etc.)
#define COLOR_ORDER   GRB     // Color order (GRB for WS2812B, RGB for others)

#define TASK_SLEEP_MS 50

CRGB leds[NUM_LEDS];

typedef struct
{
    int value;
    int brightness;
    TaskHandle_t task;
} leds_t;

leds_t m_leds = {
    .value      = 20,
    .brightness = 80,
};

void ledTask(void *pvParameters)
{
    while (1)
    {
        bool update     = false;

        long randNumber = random(0, 255) % 255;

        if (m_leds.value > randNumber)
        {
            update = true;
        }

        if (update)
        {
            if (m_leds.value > 128)
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

void ledsSetup()
{
    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

    // Example: Light up the strip with a rainbow effect
    for (int i = 0; i < (NUM_LEDS - 0); i++)
    {
        leds[i] = CHSV((i * 255) / NUM_LEDS, 255, 255); // Hue, Saturation, Value
    }

    randomSeed(analogRead(0));

    FastLED.setBrightness(m_leds.brightness);
    FastLED.show();

    xTaskCreate(ledTask, "LedTask", 1000, NULL, 1, &m_leds.task);
}

void ledsLoop()
{
}

void ledsSet(int value)
{
    m_leds.value = value;
}

void ledsSetBrightness(int value)
{
    myPrintf("brightness %d", value);

    FastLED.setBrightness(value);
    FastLED.show();
}

void ledsChangeBrightness()
{
    static bool up = false;

    if (up)
    {
        m_leds.brightness += 20;
        up = m_leds.brightness > 255 ? false : true;
    }
    else
    {
        m_leds.brightness -= 20;
        up = m_leds.brightness < 0 ? true : false;
    }

    m_leds.brightness = MAX(m_leds.brightness, 0);
    m_leds.brightness = MIN(m_leds.brightness, 255);

    ledsSetBrightness(m_leds.brightness);
}
