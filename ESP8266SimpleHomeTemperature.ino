#include "Config.h"

#include <ESP8266WiFi.h>
#include "src/Mod_ESP8266Ping.h"
#include <ESP8266WebServer.h>
#include <ESP8266SSDP.h>
#include <LittleFS.h>
#include <Wire.h>
#include <DHT_U.h>
#include <SparkFun_ENS160.h>
#include <Adafruit_AHTX0.h>
#include "Connectivity.h"
#include "Routes.h"
#include "Files.h"
#include "Logging.h"

ESP8266WebServer server(80);
Routes routes(&server);

DHT_Unified dht(4, DHT22);
SparkFun_ENS160 ens160;
Adafruit_AHTX0 aht;

bool hasEns = false;
bool hasAht = false;

unsigned long lastMillis = 0;
float temperature = 0;
float humidity = 0;
float eCo2 = 0;
float aqi = 0;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, 0);

  #ifdef LOGGING
  Serial.begin(9600);
  #endif

  LittleFS.begin();
  Wire.begin();
  dht.begin();
  hasEns = ens160.begin();
  hasAht = aht.begin();

  if (hasEns) {
    ens160.setOperatingMode(SFE_ENS160_RESET);
    delay(100);
    ens160.setOperatingMode(SFE_ENS160_STANDARD);
  }

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
  sensors_event_t temperatureEvent, humidityEvent;

  if (hasAht) {
    aht.getEvent(&humidityEvent, &temperatureEvent);
  } else {
    dht.temperature().getEvent(&temperatureEvent);
    dht.humidity().getEvent(&humidityEvent);
  }

  if (!isnan(temperatureEvent.temperature)) {
    temperature = temperatureEvent.temperature;
  }

  if (!isnan(humidityEvent.relative_humidity)) {
    humidity = humidityEvent.relative_humidity;
  }

  if (hasEns && ens160.checkDataStatus()) {
    eCo2 = ens160.getECO2();
    aqi = ens160.getAQI();
  }
}

void handleCommands() {
  char message[512];

  snprintf_P(
    message,
    sizeof(message),
    PSTR(
      "{"
        "\"commands\":{"
          "\"temperature\":{\"icon\": \"thermometer\",\"title\":\"%g °C\",\"summary\":\"Temperature\", \"mode\": \"none\"},"
          "\"humidity\":{\"icon\": \"hygrometer\",\"title\":\"%g %%\",\"summary\":\"Humidity\", \"mode\": \"none\"},"
          "\"humidity\":{\"icon\": \"gauge\",\"title\":\"%g ppm\",\"summary\":\"eCO2\", \"mode\": \"none\"},"
          "\"humidity\":{\"icon\": \"gauge\",\"title\":\"%g\",\"summary\":\"AQI\", \"mode\": \"none\"}"
        "}"
      "}"
    ),
    temperature,
    humidity,
    eCo2,
    aqi
  );

  server.keepAlive(false);
  server.send(200, F("application/json"), message);
}
