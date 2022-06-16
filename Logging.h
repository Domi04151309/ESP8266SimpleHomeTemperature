#ifndef LOGGING_H
#define LOGGING_H

#include "Config.h"

#ifdef LOGGING
  #include <Arduino.h>
  #define log(message) Serial.printf("[%d s] %s\n", millis() / 1000, message)
#else
  #define log(message)
#endif

#endif
