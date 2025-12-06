#include "settings.h"

#include <Arduino.h>
#include <pins_arduino.h>

String getSerialId()
{
    return String((uint32_t)ESP.getEfuseMac(), HEX);
}

String getUniqueId()
{
    return String(String(DEVICE_ID) + "-" + getSerialId());
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
