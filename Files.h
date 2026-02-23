#ifndef FILES_H
#define FILES_H

#include <Arduino.h>

String readFromFile(const char* filename);
bool writeToFile(const char* filename, const char* content);

#endif
