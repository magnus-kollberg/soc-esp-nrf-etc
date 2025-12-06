#include <Arduino.h>

#define BUTTON_PIN 0

volatile bool buttonPressed = false;

void IRAM_ATTR handleButtonPress()
{
    buttonPressed = true;
}

void buttonSetup()
{
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), handleButtonPress, FALLING);
}

bool buttonLoop()
{
    bool pressed = false;

    if (buttonPressed)
    {
        Serial.println("Button Pressed!");
        buttonPressed = false;
        pressed       = true;
    }

    return pressed;
}
