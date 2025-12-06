#include "settings.h"

#if SENSOR_BME280
#include <Arduino.h>

#define TINY_BME280_I2C
#include <TinyBME280.h>
tiny::BME280 sensorBme280;

float valueTemperature = -99;
float valueHumidity    = -99;
float valuePressure    = -99;

bool sensorOk          = false;

bool sensorChanged(float oldValue, float newValue)
{
    return abs(oldValue - newValue) > 0.5;
}

void sensorBme280Setup()
{
    sensorOk = false;

    // BME280
    sensorBme280.setI2CAddress(0x76);
    if (sensorBme280.begin() == false)
    {
        Serial.println("Error, the sensor did not respond. Please check wiring.");
        sensorOk = false;
    }
    else
    {
        Serial.println("Sensor BME280 is OK");
        sensorOk = true;
    }
}

bool sensorBme280Update()
{
    bool changed = false;

    if (sensorOk)
    {
        float temp = sensorBme280.readFixedTempC() / 100.0;
        float humi = sensorBme280.readFixedHumidity() / 1000.0;
        float pres = sensorBme280.readFixedPressure() / 100.0;

        if (sensorChanged(valueTemperature, temp))
        {
            valueTemperature = temp;
            changed          = true;
        }

        if (sensorChanged(valueHumidity, humi))
        {
            valueHumidity = humi;
            changed       = true;
        }

        if (sensorChanged(valuePressure, pres))
        {
            valuePressure = pres;
            changed       = true;
        }
    }
    else
    {
        valueTemperature = (float)(((int)valueTemperature + rand()) % 50);
    }

    if (changed)
    {
        Serial.println("New T: " + String(valueTemperature) + " H: " + String(valueHumidity) + " P: " + String(valuePressure));
    }

    return changed;
}
#endif