#ifndef LOGGING_H
#define LOGGING_H

#include "Config.h"

#ifdef LOGGING
  #include <Arduino.h>
  #define LOG(...) Serial.printf(__VA_ARGS__)
#else
  #define LOG(...)
#endif

#endif
