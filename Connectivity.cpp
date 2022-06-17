#include "Connectivity.h"

#include <ESP8266WiFi.h>
#include "Files.h"
#include "Logging.h"
#include "Config.h"

void configureNetwork() {
  log("Configuring network...");
  char* ssid = readFromFile("ssid");
  char* password = readFromFile("password");
  wifi_set_sleep_type(NONE_SLEEP_T);
  if (strlen(ssid) == 0 || strlen(password) == 0) {
    startAP();
  } else {
    log("Attempting to connect...");
    char* roomName = readFromFile("room_name");
    char* customHostname = (char*) malloc(sizeof(char) * 64);
    sprintf_P(customHostname, PSTR("ESP8266-SimpleHome-%s"), SAVED_OR_DEFAULT_ROOM_NAME(roomName));
    WiFi.mode(WIFI_STA);
    WiFi.hostname(customHostname);
    WiFi.begin(ssid, password);
    free(roomName);
    free(customHostname);
    for (uint8_t i = 0; i < 50 && WiFi.status() != WL_CONNECTED; i++) delay(100);
    if (WiFi.status() != WL_CONNECTED) {
      log("Failed to connect");
      WiFi.disconnect();
      startAP();
    } else {
      #ifdef LOGGING
      char* logMessage = (char*) malloc(sizeof(char) * 64);
      sprintf(logMessage, "Connected to %s", WiFi.SSID());
      log(logMessage);
      sprintf(logMessage, "IP address: %s", WiFi.localIP().toString().c_str());
      log(logMessage);
      sprintf(logMessage, "Signal strength (RSSI): %ld dBm\n", WiFi.RSSI());
      log(logMessage);
      free(logMessage);
      #endif
    }
  }
  free(ssid);
  free(password);
}

void startAP() {
  log("Starting access point...");
  IPAddress apIP(192, 168, 1, 1);
  IPAddress netMsk(255, 255, 255, 0);
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, netMsk);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
}

uint8_t RSSIToPercent(long rssi) {
  if (rssi >= -50) return 100;
  else if (rssi <= -100) return 0;
  else return (rssi + 100) * 2;
}
