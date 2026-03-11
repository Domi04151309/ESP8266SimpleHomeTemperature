#include "Config.h"

#include <Adafruit_AHTX0.h>
#include <DHT_U.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <LittleFS.h>
#include <SparkFun_ENS160.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include "Connectivity.h"
#include "Logging.h"
#include "Routes.h"

ESP8266WebServer server(80);
Routes routes(server);
WiFiUDP heartbeatUdp;

DHT_Unified dht(14, DHT22);
Adafruit_AHTX0 aht;
SparkFun_ENS160 ens160;

bool hasAht = false;
bool hasEns = false;

unsigned long lastMillis = 0;
float temperature = 0;
float humidity = 0;
uint16_t eCo2 = 0;
uint8_t aqi = 0;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, 0);

  #ifdef LOGGING
  Serial.begin(9600);
  #endif

  LittleFS.begin();
  Wire.begin();
  dht.begin();
  hasAht = aht.begin();
  hasEns = ens160.begin();

  if (hasEns) {
    ens160.setOperatingMode(SFE_ENS160_RESET);
    delay(100);
    ens160.setOperatingMode(SFE_ENS160_STANDARD);
  }

  delay(500);

  //Configuring AP
  configureNetwork();

  //Add routes
  routes.begin();
  server.on(F("/commands"), HTTP_GET, handleCommands);
  server.begin();

  // Set initial sensor data
  updateSensorData();

  digitalWrite(LED_BUILTIN, 1);
}

void loop() {
  server.handleClient();

  if (millis() - lastMillis >= UPDATE_INTERVAL) {
    lastMillis = millis();

    updateSensorData();
    sendHeartbeat(heartbeatUdp);

    LOG(
      "WiFi: %s (%d %%) | Heap: %d %% | Frag: %d %%\n",
      WiFi.status() == WL_CONNECTED ? "OK" : "LOST",
      rssiToPercent(WiFi.RSSI()),
      (int)((ESP.getFreeHeap() * 100) / 81920),
      ESP.getHeapFragmentation()
    );
  }

  delay(LOOP_DELAY);
}

void updateSensorData() {
  sensors_event_t temperatureEvent, humidityEvent;

  dht.temperature().getEvent(&temperatureEvent);
  dht.humidity().getEvent(&humidityEvent);

  if (!isnan(temperatureEvent.temperature)) {
    temperature = temperatureEvent.temperature;
  }

  if (!isnan(humidityEvent.relative_humidity)) {
    humidity = humidityEvent.relative_humidity;
  }

  if (hasAht) {
    aht.getEvent(&humidityEvent, &temperatureEvent);
    ens160.setTempCompensationCelsius(temperatureEvent.temperature);
    ens160.setRHCompensationFloat(humidityEvent.relative_humidity);
  } else {
    ens160.setTempCompensationCelsius(temperature);
    ens160.setRHCompensationFloat(humidity);
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
          "\"temperature\":{\"icon\": \"thermometer\",\"title\":\"%.1f °C\",\"summary\":\"Temperature\", \"mode\": \"none\"},"
          "\"humidity\":{\"icon\": \"hygrometer\",\"title\":\"%.1f %%\",\"summary\":\"Humidity\", \"mode\": \"none\"},"
          "\"eco2\":{\"icon\": \"gauge\",\"title\":\"%u ppm\",\"summary\":\"eCO2\", \"mode\": \"none\"},"
          "\"aqi\":{\"icon\": \"gauge\",\"title\":\"%u\",\"summary\":\"AQI\", \"mode\": \"none\"}"
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
