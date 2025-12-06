#include "settings.h"

#include <Arduino.h>
#include <Ticker.h>

#include "button.h"
#include "eeprom.h"
#include "http.h"
#include "leds.h"
#include "mqtt.h"
#include "ota.h"
#include "sensorBme280.h"
#include "tools.h"
#include "wifi.h"
#include "telnet.h"

Ticker tickerHeartBeat;
Ticker tickerSensorBeat;

/*******************************************
 * Various
 ********************************************/

void heartBeat()
{
    static int alive = 0;
    char msg[128];

    //   sprintf(msg, "Heart beat %d from %s switch %s clients %d", alive++,
    //   getUniqueId().c_str(), eepromData.valueSwitch,
    //   WiFi.softAPgetStationNum());
    sprintf(msg, "Heart beat %d from %s connected clients %d", alive++, getUniqueId().c_str(), WiFi.softAPgetStationNum());
    Serial.println(msg);

    ledInvertBlue();
}

void sensorBeat()
{
    bool changed = false;

    changed      = sensorBme280Update();

    if (changed)
    {
        // mqttUpdateAttributes();
    }
}

/*******************************************
 * Setup
 ********************************************/

void setup()
{
    delay(1000);

    // Serial
    Serial.begin(SERIAL_SPEED);
    Serial.println("\nHello world from " + getUniqueId() + "!");

    // EEPROM
    eepromSetup();

    // LEDs, blue and red
    //   pinMode(LED_BUILTIN, OUTPUT);
    //   digitalWrite(LED_BUILTIN, HIGH);
    //   pinMode(LED_BUILTIN_AUX, OUTPUT);

    Serial.println("eepromData.valueSwitch: " + String(eepromData.valueSwitch));

    if (!strncmp("OFF", eepromData.valueSwitch, 3))
    {
        // digitalWrite(LED_BUILTIN_AUX, HIGH);
    }
    else
    {
        // digitalWrite(LED_BUILTIN_AUX, LOW);
    }

    delay(500);

    // Blink 3 times to show that we started.
    ledBlinkBlue(3);

    // Setup our modules
    sensorBme280Setup();
    wifiSetup();
    mqttSetup();
    httpSetup();
    otaSetup();
    ledsSetup();
    buttonSetup();
    telnetSetup();

    // Let everything start up
    delay(1500);

    // Start our tickers
    heartBeat();
    tickerHeartBeat.attach(10, heartBeat);
    //   sensorBeat();
    //   tickerSensorBeat.attach(1, sensorBeat);

    wifiPrintStatus();
    wifiScan();
}

/*******************************************
 * Loop
 ********************************************/

void loop()
{
    bool pressed = buttonLoop();
    mqttLoop();
    wifiLoop();
    httpLoop();
    otaLoop();
    if (pressed)
    {
        ledsChangeBrightness();
    }
    ledsLoop();
    telnetLoop();
}
