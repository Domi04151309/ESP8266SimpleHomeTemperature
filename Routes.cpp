#include "Routes.h"

#include <Arduino.h>
#include <ESP.h>
#include <ESP8266WiFi.h>
#include "Files.h"
#include "Logging.h"
#include "Config.h"

Routes::Routes(ESP8266WebServer* webServer) {
  server = webServer;
  shouldRestart = false;
}

void Routes::handleRoot() {
  server->send(
    200, 
    "text/html",
    F(
      "<!doctype html><html>" HTML_HEAD "<body>"
      "<h1>Settings</h1>"
      "<p>Welcome to your ESP8266! What do you want to do?</p>"
      "<nav><ul><li><a href='/wifi'>Configure WiFi</a></li><li><a href='/room-name'>Change Room Name</a></li></ul></nav>"
      "</body></html>"
    )
  );
}

void Routes::handleWiFi() {
  server->sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server->sendHeader("Pragma", "no-cache");
  server->sendHeader("Expires", "-1");
  
  char* ssid = readFromFile("ssid");
  String page;
  page += F(
            "<!doctype html><html>" HTML_HEAD "<body>"
            "<h1>WiFi Configuration</h1>"
            "<h2>Available Networks</h2>"
            "<ul>"
          );
  int n = WiFi.scanNetworks();
  if (n > 0) {
    for (int i = 0; i < n; i++) {
      page += String(F("<li>")) + WiFi.SSID(i) + F("</li>");
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
  server->send(200, "text/html", page);
  free(ssid);
}

void Routes::handleWiFiSave() {
  shouldRestart = true;
  char ssid[32] = "";
  char password[32] = "";
  server->arg("ssid").toCharArray(ssid, sizeof(ssid) - 1);
  server->arg("password").toCharArray(password, sizeof(password) - 1);
  server->sendHeader("Location", "/success", true);
  server->send(302, "text/plain", "Redirect");
  log("Changed wifi config");
  writeToFile("ssid", ssid);
  writeToFile("password", password);
}

void Routes::handleRoomName() {
  server->sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server->sendHeader("Pragma", "no-cache");
  server->sendHeader("Expires", "-1");

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
  server->send(200, "text/html", page);
  free(roomName);
}

void Routes::handleRoomNameSave() {
  char roomName[32] = "";
  server->arg("name").toCharArray(roomName, sizeof(roomName) - 1);
  server->sendHeader("Location", "/success", true);
  server->send(302, "text/plain", "Redirect");
  log("Changed room name");
  writeToFile("room_name", roomName);
}

void Routes::handleSuccess() {
  server->send(
    200, 
    "text/html",
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

void Routes::handleCommand() {
  server->send(
    200, 
    "application/json",
    "{\"toast\":\"Online!\"}"
  );
}

void Routes::handleCss() {
  server->send(
    200, 
    "text/css",
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
  server->send(404, "text/plain", "404");
}
