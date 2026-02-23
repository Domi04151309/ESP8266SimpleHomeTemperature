#include "Config.h"

#ifdef LOGGING
  #define DEBUG_ESP_HTTP_SERVER
  #define ENABLE_DEBUG_PING
  #define DHT_DEBUG
#endif

#include <ESP8266WiFi.h>
#include "src/Mod_ESP8266Ping.h"
#include <ESP8266WebServer.h>
#include "src/Mod_ESP8266SSDP.h"
#include <LittleFS.h>
#include <DHT.h>
#include "Connectivity.h"
#include "Routes.h"
#include "Files.h"
#include "Logging.h"

ESP8266WebServer server(80);
DHT dht(4, DHT22);

unsigned long lastMillis = 0;
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

  delay(500);

  //Configuring AP
  configureNetwork();

  //Add routes
  static Routes routes(&server);

  server.on(F("/"), HTTP_GET, std::bind(&Routes::handleRoot, routes));
  server.on(F("/wifi"), HTTP_GET, std::bind(&Routes::handleWiFi, routes));
  server.on(F("/wifi-script"), HTTP_GET, std::bind(&Routes::handleWiFiScript, routes));
  server.on(F("/wifi-result"), HTTP_GET, std::bind(&Routes::handleWiFiResult, routes));
  server.on(F("/wifi-save"), HTTP_ANY, std::bind(&Routes::handleWiFiSave, routes));
  server.on(F("/room-name"), HTTP_GET, std::bind(&Routes::handleRoomName, routes));
  server.on(F("/room-name-save"), HTTP_ANY, std::bind(&Routes::handleRoomNameSave, routes));
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

  digitalWrite(LED_BUILTIN, 1);
}

void loop() {
  server.handleClient();

  if (millis() - lastMillis >= PING_INTERVAL) {
    lastMillis = millis();

    updateSensorData();
    Ping.ping(WiFi.gatewayIP());

    #ifdef LOGGING
    char logMessage[128];
    snprintf(
      logMessage,
      sizeof(logMessage),
      "WiFi: %s (%d%%) | Heap: %d%% | Frag: %d%%",
      WiFi.status() == WL_CONNECTED ? "OK" : "LOST",
      RSSIToPercent(WiFi.RSSI()),
      (int)((ESP.getFreeHeap() * 100) / 81920),
      ESP.getHeapFragmentation()
    );
    log(logMessage);
    #endif
  }

  delay(LOOP_DELAY);
}

void updateSensorData() {
  float event;

  event = dht.readTemperature();
  if (!isnan(event)) temperature = event;

  event = dht.readHumidity();
  if (!isnan(event)) humidity = event;
}

void handleCommands() {
  updateSensorData();

  String roomName = readFromFile("room_name");
  const char* savedOrDefaultRoomName = SAVED_OR_DEFAULT_ROOM_NAME(roomName);
  char message[512];

  snprintf_P(
    message,
    sizeof(message),
    PSTR(
      "{"
        "\"commands\":{"
          "\"temperature\":{\"icon\": \"thermometer\",\"title\":\"%g °C\",\"summary\":\"Temperature in your %s\", \"mode\": \"none\"},"
          "\"humidity\":{\"icon\": \"hygrometer\",\"title\":\"%g %%\",\"summary\":\"Humidity in your %s\", \"mode\": \"none\"}"
        "}"
      "}"
    ),
    temperature,
    savedOrDefaultRoomName,
    humidity,
    savedOrDefaultRoomName
  );

  server.keepAlive(false);
  server.send(200, F("application/json"), message);
}
