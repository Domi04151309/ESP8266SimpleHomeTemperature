#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <LittleFS.h>
#include "Routes.h"
#include "Logging.h"
#include "Config.h"

ESP8266WebServer server(80);

void setup() {
  delay(1000);
  //Preparations
  Serial.begin(9600);
  LittleFS.begin();
  delay(1000);

  //Configuring AP
  configureNetwork();

  //Add routes
  Routes routes(&server);
  server.on("/", std::bind(&Routes::handleRoot, routes));
  server.on("/wifi", std::bind(&Routes::handleWiFi, routes));
  server.on("/wifi-save", std::bind(&Routes::handleWiFiSave, routes));
  server.on("/room-name", std::bind(&Routes::handleRoomName, routes));
  server.on("/room-name-save", std::bind(&Routes::handleRoomNameSave, routes));
  server.on("/success", std::bind(&Routes::handleSuccess, routes));
  server.onNotFound(std::bind(&Routes::handleNotFound, routes));
  server.begin();
}

void loop() {
  server.handleClient();
  delay(100);
}

void configureNetwork() {
  log("Starting access point...");
  IPAddress apIP(192, 168, 1, 1);
  IPAddress netMsk(255, 255, 255, 0);
  WiFi.softAPConfig(apIP, apIP, netMsk);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
}
