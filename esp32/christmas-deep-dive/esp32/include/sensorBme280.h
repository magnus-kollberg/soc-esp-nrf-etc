#ifndef __sensor_bme280_h__
#define __sensor_bme280_h__

extern float valueTemperature;
extern float valueHumidity;
extern float valuePressure;

void sensorBme280Setup();
bool sensorBme280Update();

#endif