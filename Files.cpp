#include "Files.h"

#include <Arduino.h>
#include <LittleFS.h>

String readFromFile(const char* filename) {
  File file = LittleFS.open(filename, "r");

  if (!file || file.isDirectory()) {
    return String("");
  }

  String result = file.readString();
  file.close();
  return result;
}

bool writeToFile(const char* filename, const char* content) {
  File file = LittleFS.open(filename, "w");

  if (!file) {
    return false;
  }

  if (file.print(content) == 0) {
    return false;
  }

  file.close();

  return true;
}
