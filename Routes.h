#ifndef ROUTES_H
#define ROUTES_H

#include <Arduino.h>
#include <ESP8266WebServer.h>

class Routes {
  public:
    Routes(ESP8266WebServer &webServer);
    void begin();
  private:
    ESP8266WebServer &server;
    bool shouldRestart;
    void handleRoot();
    void handleWiFi();
    void handleWiFiScript();
    void handleWiFiResult();
    void handleWiFiSave();
    void handleRoomName();
    void handleRoomNameSave();
    void handleRequestRestart();
    void handleStatus();
    void handleCss();
    void handleNotFound();
    void sendPage(const String &body, int code = 200);
};

#endif
