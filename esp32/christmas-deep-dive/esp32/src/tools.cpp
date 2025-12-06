#include "settings.h"

#include <Arduino.h>
#include <pins_arduino.h>

String getSerialId()
{
    return String((uint32_t)ESP.getEfuseMac(), HEX);
}

String getUniqueId()
{
    return String(String(DEVICE_ID) + "_" + getSerialId());
}

void ledInvertBlue()
{
    // digitalWrite(LED_BUILTIN, !(digitalRead(LED_BUILTIN)));
}

void ledBlinkBlue(int nbrOfBlinks)
{
    // digitalWrite(LED_BUILTIN, HIGH);
    // for (int i=0; i<(nbrOfBlinks*2); i++) {
    //   ledInvertBlue();
    //   delay(200);
    // }
}

void ledInvertRed()
{
    // digitalWrite(LED_BUILTIN_AUX, !(digitalRead(LED_BUILTIN_AUX)));
}

bool isIp(String str)
{
    for (size_t i = 0; i < str.length(); i++)
    {
        int c = str.charAt(i);
        if (c != '.' && (c < '0' || c > '9'))
        {
            return false;
        }
    }
    return true;
}

String toStringFromIp(IPAddress ip)
{
    return ip.toString();
}

IPAddress toIpFromString(String ip)
{
    IPAddress ipAddress;
    ipAddress.fromString(ip);
    return ipAddress;
}

int myPrintf(const char *format, ...)
{
    char buffer[128];
    va_list args;
    va_start(args, format);
    int len = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    Serial.println(buffer);
    return len;
    return 0;
}