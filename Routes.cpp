#include "Routes.h"

#include <cstdint>
#include <Arduino.h>
#include <ESP.h>
#include <ESP8266WiFi.h>
#include "Files.h"
#include "Connectivity.h"
#include "Logging.h"
#include "Config.h"

Routes::Routes(ESP8266WebServer* webServer) {
  server = webServer;
}

bool Routes::shouldRestart = false;

void Routes::handleRoot() {
  server->keepAlive(false);
  server->send(
    200,
    MIME_HTML,
    F(
      "<!doctype html><html>" HTML_HEAD "<body>"
      "<h1>Settings</h1>"
      "<p>Welcome to your ESP8266! What do you want to do?</p>"
      "<nav><ul>"
      "<li><a href='/wifi'>Configure WiFi</a></li>"
      "<li><a href='/room-name'>Change Room Name</a></li>"
      "<li><a href='/weather'>Toggle weather display</a></li>"
      "<li><a href='/status'>View the device's status</a></li>"
      "</ul></nav>"
      "</body></html>"
    )
  );
}

void Routes::handleWiFi() {
  char* ssid = readFromFile("ssid");
  String page;
  page += F(
            "<!doctype html><html>" HTML_HEAD "<body>"
            "<h1>WiFi Configuration</h1>"
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
            "<p>You may want to <a href='/'>return to the home page</a>.</p>"
            "<script src='/wifi-script' defer></script>"
            "</body></html>"
          );

  server->sendHeader(F("Cache-Control"), F("no-cache, no-store, must-revalidate"));
  server->sendHeader(F("Expires"), F("0"));
  server->keepAlive(false);
  server->send(200, MIME_HTML, page);
  free(ssid);
}

void Routes::handleWiFiScript() {
  server->keepAlive(false);
  server->send(
    200,
    F("text/javascript"),
    F(
      "const list = document.getElementById('list');"
      "loadNetworks();"
      "function loadNetworks() {"
        "fetch('/wifi-result').then(response => {"
          "if (!response.ok) console.error(response.status);"
          "else return response.json();"
        "}).then(json => {"
          "list.innerHTML = '';"
          "json.forEach(element => appendItem(element));"
        "}).catch(error => {"
          "console.error(error);"
          "list.innerHTML = '';"
          "appendItem('An error occurred');"
        "});"
      "}"
      "function appendItem(text) {"
        "const li = document.createElement('li');"
        "li.appendChild(document.createTextNode(text));"
        "list.appendChild(li);"
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

  server->keepAlive(false);
  server->send(200, F("application/json"), page);
}

void Routes::handleWiFiSave() {
  Routes::shouldRestart = true;
  char ssid[32] = "";
  char password[32] = "";
  server->arg("ssid").toCharArray(ssid, sizeof(ssid) - 1);
  server->arg("password").toCharArray(password, sizeof(password) - 1);
  server->keepAlive(false);
  server->send(
    200,
    MIME_HTML,
    F(
      "<!doctype html><html>" HTML_HEAD "<body>"
      "<h1>Success</h1>"
      "<p>Updated WiFi settings successfully! Your device will restart now!</p>"
      "<script src='/request-restart' defer></script>"
      "</body></html>"
    )
  );
  writeToFile("ssid", ssid);
  writeToFile("password", password);
  log("Changed wifi config");
}

void Routes::handleRoomName() {
  char* roomName = readFromFile("room_name");
  String page;
  page += F(
            "<!doctype html><html>" HTML_HEAD "<body>"
            "<h1>Room Name</h1>"
            "<p>The current room name is &ldquo;"
          );
  page += SAVED_OR_DEFAULT_ROOM_NAME(roomName);
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
            "<p>You may want to <a href='/'>return to the home page</a>.</p>"
            "</body></html>"
          );

  server->sendHeader(F("Cache-Control"), F( "no-cache, no-store, must-revalidate"));
  server->sendHeader(F("Expires"), F("0"));
  server->keepAlive(false);
  server->send(200, MIME_HTML, page);
  free(roomName);
}

void Routes::handleRoomNameSave() {
  char roomName[32] = "";
  server->arg("name").toCharArray(roomName, sizeof(roomName) - 1);
  server->keepAlive(false);
  server->send(
    200,
    MIME_HTML,
    F(
      "<!doctype html><html>" HTML_HEAD "<body>"
      "<h1>Success</h1>"
      "<p>Updated the room name successfully!</p>"
      "<p>You may want to <a href='/'>return to the home page</a>.</p>"
      "</body></html>"
    )
  );
  writeToFile("room_name", roomName);
  log("Changed room name");
}

void Routes::handleWeather() {
  char* weatherDisplay = readFromFile("weather");
  String page;
  page += F(
            "<!doctype html><html>" HTML_HEAD "<body>"
            "<h1>Weather Display</h1>"
            "<p>Currently the weather display is "
          );
  page += strcmp(weatherDisplay, "1") == 0 ? "enabled" : "disabled";
  page += F(
            ". Please note that weather data is fetched from the Internet. "
            "Data that can be regarded personal will get transmitted to the weather provider.</p>"
            "<h2>Toggle Weather Display</h2>"
            "<form method='POST' action='weather-save'>"
            "<input type='hidden' name='bool' value='"
          );
  page += strcmp(weatherDisplay, "1") == 0 ? "0" : "1";
  page += F(
            "' />"
            "<input type='submit' value='Toggle' />"
            "</form>"
            "<p>You may want to <a href='/'>return to the home page</a>.</p>"
            "</body></html>"
          );

  server->sendHeader(F("Cache-Control"), F( "no-cache, no-store, must-revalidate"));
  server->sendHeader(F("Expires"), F("0"));
  server->keepAlive(false);
  server->send(200, MIME_HTML, page);
  free(weatherDisplay);
}

void Routes::handleWeatherSave() {
  char weatherDisplay[32] = "";
  server->arg("bool").toCharArray(weatherDisplay, sizeof(weatherDisplay) - 1);
  server->keepAlive(false);
  server->send(
    200,
    MIME_HTML,
    F(
      "<!doctype html><html>" HTML_HEAD "<body>"
      "<h1>Success</h1>"
      "<p>Updated the weather display successfully!</p>"
      "<p>You may want to <a href='/'>return to the home page</a>.</p>"
      "</body></html>"
    )
  );
  writeToFile("weather", weatherDisplay);
  log("Changed weather display");
}

void Routes::handleRequestRestart() {
  server->keepAlive(false);
  server->send(200, F("text/javascript"), F("console.log('Restarting');"));
  if (Routes::shouldRestart) {
    delay(2000);
    ESP.restart();
  }
}

void Routes::handleStatus() {
  char* uptime = (char*) malloc(sizeof(char) * 16);
  uint32_t seconds = millis() / 1000;
  uint32_t minutes = seconds / 60;
  uint16_t hours = minutes / 60;
  sprintf_P(uptime, PSTR("%02u:%02u:%02u"), hours, minutes % 60, seconds % 60);
  
  String page;
  page += F(
            "<!doctype html><html>" HTML_HEAD "<body>"
            "<h1>Status</h1>"
            "<ul>"
            "<li>WiFi: "
          );
  page += WiFi.status() == WL_CONNECTED ? WiFi.SSID() : "Disconnected";
  page += F(
            "</li>"
            "<li>Signal Strength: "
          );
  page += RSSIToPercent(WiFi.RSSI());
  page += F(
            " %</li>"
            "<li>RAM Usage: "
          );
  page += (ESP.getFreeHeap() * 100) / 64000 * (-1) + 100;
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
            "<p>You may want to <a href='/'>return to the home page</a>.</p>"
            "</body></html>"
          );

  server->sendHeader(F("Cache-Control"), F("no-cache, no-store, must-revalidate"));
  server->sendHeader(F("Expires"), F("0"));
  server->keepAlive(false);
  server->send(200, MIME_HTML, page);
  free(uptime);
}

void Routes::handleCommand() {
  server->keepAlive(false);
  server->send(
    200,
    F("application/json"),
    F(
      "{\"toast\":\"Online!\"}"
    )
  );
}

void Routes::handleCss() {
  server->keepAlive(false);
  server->send(
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
  server->keepAlive(false);
  server->send(
    404,
    MIME_HTML,
    F(
      "<!doctype html><html>" HTML_HEAD "<body>"
      "<h1>404</h1>"
      "<p>Not found!</p>"
      "<p>You may want to <a href='/'>return to the home page</a>.</p>"
      "</body></html>"
    )
  );
}
