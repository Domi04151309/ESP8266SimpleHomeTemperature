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
  shouldRestart = false;
}

void Routes::handleRoot() {
  server->keepAlive(false);
  server->send(
    200, 
    MIME_HTML,
    F(
      "<!doctype html><html>" HTML_HEAD "<body>"
      "<h1>Settings</h1>"
      "<p>Welcome to your ESP8266! What do you want to do?</p>"
      "<nav><ul><li><a href='/wifi'>Configure WiFi</a></li><li><a href='/room-name'>Change Room Name</a></li><li><a href='/status'>View the device's status</a></li></ul></nav>"
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
            "<ul>"
          );
  uint8_t n = WiFi.scanNetworks();
  if (n > 0) {
    for (uint8_t i = 0; i < n; i++) {
      page += F("<li>");
      page += WiFi.SSID(i);
      page += F("</li>");
    }
  } else {
    page += F("<li>No networks found</li>");
  }
  page += F(
            "</ul>"
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
            "</body></html>"
          );
          
  server->sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server->sendHeader("Expires", "0");
  server->keepAlive(false);
  server->send(200, MIME_HTML, page);
  free(ssid);
}

void Routes::handleWiFiSave() {
  shouldRestart = true;
  char ssid[32] = "";
  char password[32] = "";
  server->arg("ssid").toCharArray(ssid, sizeof(ssid) - 1);
  server->arg("password").toCharArray(password, sizeof(password) - 1);
  server->sendHeader("Location", "/success", true);
  server->keepAlive(false);
  server->send(302, "text/plain", "Redirect");
  log("Changed wifi config");
  writeToFile("ssid", ssid);
  writeToFile("password", password);
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
          
  server->sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server->sendHeader("Expires", "0");
  server->keepAlive(false);
  server->send(200, MIME_HTML, page);
  free(roomName);
}

void Routes::handleRoomNameSave() {
  char roomName[32] = "";
  server->arg("name").toCharArray(roomName, sizeof(roomName) - 1);
  server->sendHeader("Location", "/success", true);
  server->keepAlive(false);
  server->send(302, "text/plain", "Redirect");
  log("Changed room name");
  writeToFile("room_name", roomName);
}

void Routes::handleSuccess() {
  server->keepAlive(false);
  server->send(
    200, 
    MIME_HTML,
    F(
      "<!doctype html><html>" HTML_HEAD "<body>"
      "<h1>Success</h1>"
      "<p>Updated settings successfully!</p>"
      "<p>You may want to <a href='/'>return to the home page</a>.</p>"
      "</body></html>"
    )
  );
  delay(3000);
  if (shouldRestart) ESP.restart();
}

void Routes::handleStatus() {
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
            #ifdef LOGGING
            "<li>LOGGING IS ENABLED</li>"
            #endif
            "</ul>"
            "<p>You may want to <a href='/'>return to the home page</a>.</p>"
            "</body></html>"
          );
          
  server->sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server->sendHeader("Expires", "0");
  server->keepAlive(false);
  server->send(200, MIME_HTML, page);
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
  delay(3000);
  if (shouldRestart) ESP.restart();
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
