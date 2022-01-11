#include <ESP8266WebServer.h>
#include <LittleFS.h>
#include "Connectivity.h"
#include "Routes.h"
#include "Config.h"

ESP8266WebServer server(80);

void setup() {
  delay(1000);
  #ifdef LOGGING
  Serial.begin(9600);
  #endif
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
