#ifndef __log_h__
#define __log_h__

#include <Arduino.h>

#define ENABLED(_flag) ((defined(_flag##_ENABLED) && (_flag##_ENABLED)) ? 1 : 0)

#define ENTER()        logMessage("%s(%d) enter", __func__, __LINE__)
#define EXIT()         logMessage("%s(%d) exit", __func__, __LINE__)

void logSetup(unsigned long baudRate);
void logMessage(const char *format, ...);

String getFormattedTime(void);

#endif