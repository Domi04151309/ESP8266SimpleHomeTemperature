#include "Config.h"

#ifdef LOGGING
  #define DEBUG_ESP_HTTP_SERVER
#endif

#include <ESP.h>
#include <ESP8266WiFi.h>
#include <ESP8266Ping.h>
#include <ESP8266WebServer.h>
#include <ESP8266SSDP.h>
#include <LittleFS.h>
#include <DHT_U.h>
#include "Connectivity.h"
#include "Routes.h"
#include "Files.h"
#include "Logging.h"

ESP8266WebServer server(80);
DHT_Unified dht(4, DHT22);

unsigned int cycle = 0;
float temperature = 0;
float humidity = 0;

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
  server.on(F("/"), HTTP_GET, std::bind(&Routes::handleRoot, routes));
  server.on(F("/wifi"), HTTP_GET, std::bind(&Routes::handleWiFi, routes));
  server.on(F("/wifi-save"), HTTP_GET, std::bind(&Routes::handleWiFiSave, routes));
  server.on(F("/room-name"), HTTP_GET, std::bind(&Routes::handleRoomName, routes));
  server.on(F("/room-name-save"), HTTP_GET, std::bind(&Routes::handleRoomNameSave, routes));
  server.on(F("/success"), HTTP_GET, std::bind(&Routes::handleSuccess, routes));
  server.on(F("/status"), HTTP_GET, std::bind(&Routes::handleStatus, routes));
  server.on(F("/commands"), HTTP_GET, handleCommands);
  server.on(F("/temperature"), HTTP_GET, std::bind(&Routes::handleCommand, routes));
  server.on(F("/humidity"), HTTP_GET, std::bind(&Routes::handleCommand, routes));
  server.on(F("/css"), HTTP_GET, std::bind(&Routes::handleCss, routes));
  server.on(F("/description.xml"), HTTP_GET, []() {
    WiFiClient client = server.client();
    SSDP.schema(client);
    client.stop();
  });
  server.onNotFound(std::bind(&Routes::handleNotFound, routes));
  server.begin();

  //Service Discovery
  SSDP.setSchemaURL(F("description.xml"));
  SSDP.setHTTPPort(80);
  SSDP.setName(F("Thermometer"));
  SSDP.setSerialNumber(F("0"));
  SSDP.setURL(F("commands"));
  SSDP.setModelName(F("SimpleHome"));
  SSDP.setModelNumber(F("0"));
  SSDP.setModelURL(F("https://github.com/Domi04151309/HomeApp"));
  SSDP.setManufacturer(F("none"));
  SSDP.setManufacturerURL(F("about:blank"));
  SSDP.setDeviceType(F("upnp:rootdevice"));
  SSDP.begin();
  
  digitalWrite(LED_BUILTIN, 1);
}

void loop() {
  server.handleClient();
  
  if ((cycle * LOOP_DELAY) / PING_INTERVAL >= 1) { 
    cycle = 0;
    bool networkAccess = Ping.ping(WiFi.gatewayIP(), 1);
    
    #ifdef LOGGING
    char* logMessage = (char*) malloc(sizeof(char) * 64);   
    sprintf(logMessage, "WiFi Status:        %s (%d %%) %s", WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected", RSSIToPercent(WiFi.RSSI()), networkAccess ? "" : "without access");
    log(logMessage);
    sprintf(logMessage, "Heap Usage:         %d %%", (ESP.getFreeHeap() * 100) / 64000 * (-1) + 100);
    log(logMessage);
    sprintf(logMessage, "Heap Fragmentation: %d %%\n", ESP.getHeapFragmentation());
    log(logMessage);
    free(logMessage);
    #endif
  }
  cycle++;
  
  delay(LOOP_DELAY);
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
  char* message = (char*) malloc(sizeof(char) * 256);
  sprintf(
    message, 
    "{\"commands\":{\"temperature\":{\"icon\": \"thermometer\",\"title\":\"%g °C\",\"summary\":\"Temperature in your %s\", \"mode\": \"none\"},\"humidity\":{\"icon\": \"hygrometer\",\"title\":\"%g %%\",\"summary\":\"Humidity in your %s\", \"mode\": \"none\"}}}",
    temperature,
    SAVED_OR_DEFAULT_ROOM_NAME(roomName),
    humidity,
    SAVED_OR_DEFAULT_ROOM_NAME(roomName)
  );

  server.keepAlive(false);
  server.send(200, F("application/json"), message);
  free(roomName);
  free(message);
}
