#include "Connectivity.h"

#include <ESP8266WiFi.h>
#include "Files.h"
#include "Logging.h"
#include "Config.h"

void configureNetwork() {
  LOG("Configuring network...\n");

  String ssid = readFromFile("ssid");
  String password = readFromFile("password");

  wifi_set_sleep_type(NONE_SLEEP_T);

  if (ssid.length() == 0 || password.length() == 0) {
    startAP();
  } else {
    LOG("Attempting to connect...\n");

    String roomName = readFromFile("room_name");
    char customHostname[48];
    snprintf_P(customHostname, sizeof(customHostname), PSTR("ESP8266-SimpleHome-%s"), roomName.c_str());

    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);
    WiFi.hostname(customHostname);
    WiFi.begin(ssid, password);

    for (uint8_t i = 0; i < 50; i++) {
      if (WiFi.status() == WL_CONNECTED) {
        break;
      }

      delay(200);
    }

    if (WiFi.status() != WL_CONNECTED) {
      LOG("Failed to connect\n");

      WiFi.disconnect();
      startAP();
    } else {
      LOG("Connected to %s\n", WiFi.SSID().c_str());
      LOG("IP: %s\n", WiFi.localIP().toString().c_str());
    }
  }
}

void startAP() {
  LOG("Starting access point...\n");

  IPAddress apIP(192, 168, 1, 1);
  IPAddress netMsk(255, 255, 255, 0);

  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, netMsk);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
}

uint8_t RSSIToPercent(long rssi) {
  if (rssi <= -100) {
    return 0;
  }

  if (rssi >= -50) {
    return 100;
  }

  return (uint8_t)(2 * (rssi + 100));
}
