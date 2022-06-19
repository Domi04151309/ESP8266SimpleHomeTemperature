#include "Config.h"

#ifdef LOGGING
  #define DEBUG_ESP_HTTP_SERVER
  #define ENABLE_DEBUG_PING
  #define DHT_DEBUG
#endif

#include <ESP.h>
#include <ESP8266WiFi.h>
#include "src/Mod_ESP8266Ping.h"
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include "src/Mod_ESP8266SSDP.h"
#include <LittleFS.h>
#include "src/Mod_DHT.h"
#include "Connectivity.h"
#include "Routes.h"
#include "Files.h"
#include "Logging.h"

ESP8266WebServer server(80);
DHT dht(4, DHT22);

unsigned int cycle = 0;
float temperature = 0;
float humidity = 0;
char* weather;

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
  server.on(F("/wifi-script"), HTTP_GET, std::bind(&Routes::handleWiFiScript, routes));
  server.on(F("/wifi-result"), HTTP_GET, std::bind(&Routes::handleWiFiResult, routes));
  server.on(F("/wifi-save"), HTTP_ANY, std::bind(&Routes::handleWiFiSave, routes));
  server.on(F("/room-name"), HTTP_GET, std::bind(&Routes::handleRoomName, routes));
  server.on(F("/room-name-save"), HTTP_ANY, std::bind(&Routes::handleRoomNameSave, routes));
  server.on(F("/weather"), HTTP_GET, std::bind(&Routes::handleWeather, routes));
  server.on(F("/weather-save"), HTTP_ANY, std::bind(&Routes::handleWeatherSave, routes));
  server.on(F("/request-restart"), HTTP_GET, std::bind(&Routes::handleRequestRestart, routes));
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
  strcpy_P(SSDP.schemaURL, PSTR("description.xml"));
  SSDP.port = 80;
  strcpy_P(SSDP.friendlyName, PSTR("Thermometer"));
  strcpy_P(SSDP.presentationURL, PSTR("status"));
  strcpy_P(SSDP.modelName, PSTR("SimpleHome"));
  strcpy_P(SSDP.modelNumber, PSTR("0"));
  strcpy_P(SSDP.modelURL, PSTR("https://github.com/Domi04151309/HomeApp"));
  strcpy_P(SSDP.deviceType, PSTR("upnp:rootdevice"));
  SSDP.begin();

  //Weather service
  weather = (char*) malloc(sizeof(char) * 64);
  strcpy_P(weather, PSTR("Unknown"));

  digitalWrite(LED_BUILTIN, 1);
}

void loop() {
  server.handleClient();

  if ((cycle * LOOP_DELAY) / PING_INTERVAL >= 1) {
    cycle = 0;
    char* weatherDisplay = readFromFile("weather");
    if (strcmp(weatherDisplay, "1") == 0) {
      HTTPClient http;
      WiFiClientSecure client;
      client.setInsecure(); 
      http.begin(client, "wttr.in", 443, "/?T&format=%t+in+%l", true);
      if (http.GET() == HTTP_CODE_OK) strcpy(weather, http.getString().c_str());
      http.end();
    } else {
      Ping.ping(WiFi.gatewayIP());
    }
    free(weatherDisplay);

    #ifdef LOGGING
    char* logMessage = (char*) malloc(sizeof(char) * 64);
    sprintf(logMessage, "WiFi Status:        %s (%d %%)", WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected", RSSIToPercent(WiFi.RSSI()));
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
  float event;
  
  event = dht.readTemperature();
  if (!isnan(event)) {
    temperature = event;
  }

  event = dht.readHumidity();
  if (!isnan(event)) {
    humidity = event;
  }

  char* roomName = readFromFile("room_name");
  char* weatherDisplay = readFromFile("weather");
  char* message = (char*) malloc(sizeof(char) * 512);
  sprintf_P(
    message,
    PSTR(
      "{"
        "\"commands\":{"
          "\"temperature\":{\"icon\": \"thermometer\",\"title\":\"%g °C\",\"summary\":\"Temperature in your %s\", \"mode\": \"none\"},"
          "\"humidity\":{\"icon\": \"hygrometer\",\"title\":\"%g %%\",\"summary\":\"Humidity in your %s\", \"mode\": \"none\"}"
    ),
    temperature,
    SAVED_OR_DEFAULT_ROOM_NAME(roomName),
    humidity,
    SAVED_OR_DEFAULT_ROOM_NAME(roomName),
    weather
  );
  if (strcmp(weatherDisplay, "1") == 0) {
    sprintf_P(
      message + strlen(message),
      PSTR(
        ","
        "\"weather\":{\"icon\": \"gauge\",\"title\":\"Weather\",\"summary\":\"%s\", \"mode\": \"none\"}"
      ),
      weather
    );
  }
  strcat_P(message, PSTR("}}"));

  server.keepAlive(false);
  server.send(200, F("application/json"), message);
  free(roomName);
  free(weatherDisplay);
  free(message);
}
