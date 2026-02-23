#include "Files.h"

#include <LittleFS.h>

char* readFromFile(const char* filename) {
  File file = LittleFS.open(filename, "r");

  if (!file || file.isDirectory()) {
    char* result = (char*) malloc(sizeof(char));
    result[0] = '\0';
    return result;
  }

  size_t size = file.size();
  char* result = (char*) malloc(size + 1);

  file.readBytes(result, size);
  result[size] = '\0';
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
