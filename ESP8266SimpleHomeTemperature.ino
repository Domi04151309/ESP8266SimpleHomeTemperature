#include <ESP8266WebServer.h>
#include <LittleFS.h>
#include <DHT_U.h>
#include "Connectivity.h"
#include "Routes.h"
#include "Files.h"
#include "Logging.h"
#include "Config.h"

ESP8266WebServer server(80);
DHT_Unified dht(4, DHT22);

float temperature = 0;
float humidity = 0;

#ifdef LOGGING
int cycle = 0;
#endif

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, 0);
  
  #ifdef LOGGING
  Serial.begin(9600);
  #endif
  LittleFS.begin();
  dht.begin();
  
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
  server.on("/commands", handleCommands);
  server.on("/temperature", std::bind(&Routes::handleCommand, routes));
  server.on("/humidity", std::bind(&Routes::handleCommand, routes));
  server.on("/css", std::bind(&Routes::handleCss, routes));
  server.onNotFound(std::bind(&Routes::handleNotFound, routes));
  server.begin();
  
  digitalWrite(LED_BUILTIN, 1);
}

void loop() {
  server.handleClient();

  #ifdef LOGGING
  if (cycle % 100 == 0) {
    char* logMessage = (char*) malloc(sizeof(char) * 64);
    uint32_t freeHeap = ESP.getFreeHeap();
    sprintf(logMessage, "Heap Usage:       %d %%", (freeHeap * 100) / 64000 * (-1) + 100);
    log(logMessage);
    sprintf(logMessage, "Last Temperature: %g °C", temperature);
    log(logMessage);
    sprintf(logMessage, "Last Humidity:    %g %%\n", humidity);
    log(logMessage);
    free(logMessage);
    cycle = 0;
  }
  cycle++;
  #endif
  
  delay(100);
}

void handleCommands() {
  sensors_event_t event;
  
  dht.temperature().getEvent(&event);
  if (!isnan(event.temperature)) {
    temperature = event.temperature;
  }
  
  dht.humidity().getEvent(&event);
  if (!isnan(event.relative_humidity)) {
    humidity = event.relative_humidity;
  }
  
  char* roomName = readFromFile("room_name");
  char* message = (char*) malloc(sizeof(char) * 512);
  sprintf(
    message, 
    "{\"commands\":{\"temperature\":{\"icon\": \"thermometer\",\"title\":\"%g °C\",\"summary\":\"Temperature in your %s\"},\"humidity\":{\"icon\": \"hygrometer\",\"title\":\"%g %%\",\"summary\":\"Humidity in your %s\"}}}",
    temperature,
    SAVED_OR_DEFAULT_ROOM_NAME(roomName),
    humidity,
    SAVED_OR_DEFAULT_ROOM_NAME(roomName)
  );
  
  server.send(200, "application/json", message);
  free(roomName);
  free(message);
  log("Requested commands");
}
