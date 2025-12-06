#ifndef __tools_h__
#define __tools_h__

#include "IPAddress.h"
#include "settings.h"

#define MIN(a, b) (a) > (b) ? (b) : (a)
#define MAX(a, b) (a) > (b) ? (a) : (b)

String getSerialId();
String getUniqueId();
bool isIp(String str);
String toStringFromIp(IPAddress ip);
IPAddress toIpFromString(String ip);

#endif