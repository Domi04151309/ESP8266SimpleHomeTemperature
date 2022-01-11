#include "Files.h"

#include <LittleFS.h>

char* readFromFile(const char* filename) {
  File file = LittleFS.open(filename, "r");
  if (!file) {
    char* result = (char*) malloc(sizeof(char));
    sprintf(result, "");
    return result;
  }
  char* result = (char*) malloc(file.size() + sizeof(char));
  int i;
  for (i = 0; file.available(); i++) {
      result[i] = (char) file.read();
  }
  result[i] = '\0';
  file.close();
  return result;
}

bool writeToFile(const char* filename, char content[]) {  
  File file = LittleFS.open(filename, "w");
  if (!file) return false;
  if (file.print(content) == 0) return false;
  file.close();
  return true;
}
