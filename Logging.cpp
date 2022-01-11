#include "Logging.h"

#include <Arduino.h>
#include "Config.h"

void log(const char* message) {
  #ifdef LOGGING
  Serial.printf("[%d s] %s\n", millis() / 1000, message);
  #endif
}
