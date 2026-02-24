#include "Config.h"

#include <ESP8266WiFi.h>
#include "src/Mod_ESP8266Ping.h"
#include <ESP8266WebServer.h>
#include <ESP8266SSDP.h>
#include <LittleFS.h>
#include <DHT.h>
#include "Connectivity.h"
#include "Routes.h"
#include "Files.h"
#include "Logging.h"

ESP8266WebServer server(80);
Routes routes(&server);
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
  server.on(F("/"), HTTP_GET, []() { routes.handleRoot(); });
  server.on(F("/wifi"), HTTP_GET, []() { routes.handleWiFi(); });
  server.on(F("/wifi-script"), HTTP_GET, []() { routes.handleWiFiScript(); });
  server.on(F("/wifi-result"), HTTP_GET, []() { routes.handleWiFiResult(); });
  server.on(F("/wifi-save"), HTTP_ANY, []() { routes.handleWiFiSave(); });
  server.on(F("/room-name"), HTTP_GET, []() { routes.handleRoomName(); });
  server.on(F("/room-name-save"), HTTP_ANY, []() { routes.handleRoomNameSave(); });
  server.on(F("/request-restart"), HTTP_GET, []() { routes.handleRequestRestart(); });
  server.on(F("/status"), HTTP_GET, []() { routes.handleStatus(); });
  server.on(F("/commands"), HTTP_GET, handleCommands);
  server.on(F("/css"), HTTP_GET, []() { routes.handleCss(); });
  server.on(F("/description.xml"), HTTP_GET, []() {
    WiFiClient client = server.client();
    SSDP.schema(client);
    client.stop();
  });
  server.onNotFound([]() { routes.handleNotFound(); });
  server.begin();

  //Service Discovery
  SSDP.setSchemaURL(F("description.xml"));
  SSDP.setHTTPPort(80);
  SSDP.setName(F("Thermometer"));
  SSDP.setURL(F("status"));
  SSDP.setModelName(F("SimpleHome"));
  SSDP.setModelNumber(F("0"));
  SSDP.setModelURL(F("https://github.com/Domi04151309/HomeApp"));
  SSDP.setDeviceType(F("upnp:rootdevice"));
  SSDP.begin();

  digitalWrite(LED_BUILTIN, 1);
}

void loop() {
  server.handleClient();

  if (millis() - lastMillis >= PING_INTERVAL) {
    lastMillis = millis();

    updateSensorData();
    Ping.ping(WiFi.gatewayIP());

    LOG(
      "WiFi: %s (%d %%) | Heap: %d %% | Frag: %d %%\n",
      WiFi.status() == WL_CONNECTED ? "OK" : "LOST",
      RSSIToPercent(WiFi.RSSI()),
      (int)((ESP.getFreeHeap() * 100) / 81920),
      ESP.getHeapFragmentation()
    );
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
  char message[256];

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
    roomName.c_str(),
    humidity,
    roomName.c_str()
  );

  server.keepAlive(false);
  server.send(200, F("application/json"), message);
}
