#include "Routes.h"

#include <Arduino.h>
#include <cstdint>
#include <ESP8266SSDP.h>
#include <ESP8266WiFi.h>
#include "Config.h"
#include "Connectivity.h"
#include "Files.h"
#include "Logging.h"

Routes::Routes(ESP8266WebServer &webServer): server(webServer), shouldRestart(false) {}

void Routes::begin() {
  String roomName = readFromFile("room_name");

  SSDP.setSchemaURL(F("description.xml"));
  SSDP.setHTTPPort(80);
  SSDP.setName(roomName.length() > 0 ? roomName : F("ESP8266-SimpleHome"));
  SSDP.setURL(F("status"));
  SSDP.setModelName(F("SimpleHome"));
  SSDP.setModelNumber(F("0"));
  SSDP.setModelURL(F("https://github.com/Domi04151309/HomeApp"));
  SSDP.setDeviceType(F("upnp:rootdevice"));
  SSDP.begin();

  server.on(F("/"), HTTP_GET, [this]() { this->handleRoot(); });
  server.on(F("/wifi"), HTTP_GET, [this]() { this->handleWiFi(); });
  server.on(F("/wifi-script"), HTTP_GET, [this]() { this->handleWiFiScript(); });
  server.on(F("/wifi-result"), HTTP_GET, [this]() { this->handleWiFiResult(); });
  server.on(F("/wifi-save"), HTTP_ANY, [this]() { this->handleWiFiSave(); });
  server.on(F("/room-name"), HTTP_GET, [this]() { this->handleRoomName(); });
  server.on(F("/room-name-save"), HTTP_ANY, [this]() { this->handleRoomNameSave(); });
  server.on(F("/request-restart"), HTTP_GET, [this]() { this->handleRequestRestart(); });
  server.on(F("/status"), HTTP_GET, [this]() { this->handleStatus(); });
  server.on(F("/css"), HTTP_GET, [this]() { this->handleCss(); });
  server.on(F("/description.xml"), HTTP_GET, [this]() {
    WiFiClient client = this->server.client();
    SSDP.schema(client);
    client.stop();
  });
  server.onNotFound([this]() { this->handleNotFound(); });
}

void Routes::handleRoot() {
  sendPage(
    F(
      "<h1>Settings</h1>"
      "<p>Welcome to your ESP8266! What do you want to do?</p>"
      "<ul>"
      "<li><a href='/wifi'>Configure WiFi</a></li>"
      "<li><a href='/room-name'>Change room name</a></li>"
      "<li><a href='/status'>View the device's status</a></li>"
      "</ul>"
    )
  );
}

void Routes::handleWiFi() {
  String ssid = readFromFile("ssid");

  String page;
  page += F(
            "<h1>WiFi Configuration</h1>"
            "<p>The current SSID is &ldquo;"
          );
  page += ssid;
  page += F(
            "&rdquo;.</p>"
            "<h2>Available Networks</h2>"
            "<ul id='list'><li>Loading</li></ul>"
            "<h2>Connect to a Network</h2>"
            "<form method='POST' action='wifi-save'>"
            "<input type='text' placeholder='SSID' name='ssid' value='"
          );
  page += ssid;
  page += F(
            "' required />"
            "<input type='password' placeholder='Password' name='password' required />"
            "<input type='submit' value='Connect' />"
            "</form>"
            "<script src='/wifi-script' defer></script>"
          );

  sendPage(page);
}

void Routes::handleWiFiScript() {
  server.keepAlive(false);
  server.send(
    200,
    F("text/javascript"),
    F(
      "const list = document.getElementById('list');"

      "loadNetworks();"

      "async function loadNetworks() {"
        "const response = await fetch('/wifi-result');"
        "const json = await response.json();"

        "list.replaceChildren(...json.map(item => {"
          "const li = document.createElement('li');"
          "li.textContent = item;"
          "return li;"
        "}));"
      "}"
    )
  );
}

void Routes::handleWiFiResult() {
  String page;
  page += F("[");
  uint8_t n = WiFi.scanNetworks();
  if (n > 0) {
    for (uint8_t i = 0; i < n; i++) {
      page += '"';
      page += WiFi.SSID(i);
      page += '"';
      if (i != n - 1) page += ',';
    }
  } else {
    page += F("\"No networks found\"");
  }
  page += F("]");

  server.keepAlive(false);
  server.send(200, F("application/json"), page);

  WiFi.scanDelete();
}

void Routes::handleWiFiSave() {
  shouldRestart = true;

  writeToFile("ssid", server.arg("ssid").c_str());
  writeToFile("password", server.arg("password").c_str());

  sendPage(
    F(
      "<h1>Success</h1>"
      "<p>Updated WiFi settings successfully! Your device will restart now!</p>"
      "<script src='/request-restart' defer></script>"
    )
  );
}

void Routes::handleRoomName() {
  String roomName = readFromFile("room_name");

  String page;
  page += F(
            "<h1>Room Name</h1>"
            "<p>The current room name is &ldquo;"
          );
  page += roomName;
  page += F(
            "&rdquo;.</p>"
            "<h2>Change Room Name</h2>"
            "<form method='POST' action='room-name-save'>"
            "<input type='text' placeholder='Room name' name='name' value='"
          );
  page += roomName;
  page += F(
            "' />"
            "<input type='submit' value='Change' />"
            "</form>"
          );

  sendPage(page);
}

void Routes::handleRoomNameSave() {
  writeToFile("room_name", server.arg("name").c_str());

  sendPage(
    F(
      "<h1>Success</h1>"
      "<p>Updated room name successfully!</p>"
    )
  );
}

void Routes::handleRequestRestart() {
  server.keepAlive(false);
  server.send(200, F("text/javascript"), F("console.log('Restarting');"));
  if (shouldRestart) {
    delay(1500);
    ESP.restart();
  }
}

void Routes::handleStatus() {
  char uptime[16];
  uint32_t seconds = millis() / 1000;
  uint32_t minutes = seconds / 60;
  uint16_t hours = minutes / 60;
  snprintf_P(uptime, sizeof(uptime), PSTR("%02u:%02u:%02u"), hours, minutes % 60, seconds % 60);

  String page;
  page += F(
            "<h1>Status</h1>"
            "<ul>"
            "<li>WiFi: "
          );
  page += WiFi.status() == WL_CONNECTED ? WiFi.SSID() : "Disconnected";
  page += F(
            "</li>"
            "<li>Signal Strength: "
          );
  page += rssiToPercent(WiFi.RSSI());
  page += F(
            " %</li>"
            "<li>RAM Usage: "
          );
  page += (ESP.getFreeHeap() * 100) / 81920 * (-1) + 100;
  page += F(
            " %</li>"
            "<li>RAM Fragmentation: "
          );
  page += ESP.getHeapFragmentation();
  page += F(
            " %</li>"
            "<li>Uptime: "
          );
  page += uptime;
  page += F(
            "</li>"
            #ifdef LOGGING
            "<li>LOGGING IS ENABLED</li>"
            #endif
            "</ul>"
          );

  sendPage(page);
}

void Routes::handleCss() {
  server.keepAlive(false);
  server.send(
    200,
    F("text/css"),
    F(
      ":root { font-family: sans-serif; line-height: 1.5; }"
      "* { box-sizing: border-box; font-weight: normal; }"
      "body { max-width: 480px; margin: auto; padding: 16px; }"
      "p, li { color: rgba(0, 0, 0, .6); }"
      "input { display: block; width: 100%; margin: 8px 0; }"
    )
  );
}

void Routes::handleNotFound() {
  sendPage(
    F(
      "<h1>404</h1>"
      "<p>Not found!</p>"
    ),
    404
  );
}

void Routes::sendPage(const String &body, int code) {
  String page;
  page.reserve(1024);
  page += F(
            "<!doctype html>"
            "<html>"
              "<head>"
                "<meta charset='utf-8'>"
                "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
                "<title>Settings</title>"
                "<link rel='stylesheet' href='/css'>"
              "</head>"
              "<body>"
          );
  page += body;
  page += F(
                "<p><a href='/'>Return to home page</a></p>"
              "</body>"
            "</html>"
          );

  server.keepAlive(false);
  server.sendHeader(F("Cache-Control"), F("no-cache, no-store, must-revalidate"));
  server.sendHeader(F("Expires"), F("0"));
  server.send(code, F("text/html"), page);
}
