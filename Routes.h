#ifndef ROUTES_H
#define ROUTES_H

#include <ESP8266WebServer.h>

#define HTML_HEAD "<head><meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'><title>Settings</title><link rel='stylesheet' href='/css'></head>"

class Routes {
  public:
    Routes(ESP8266WebServer* webServer);
    void handleRoot();
    void handleWiFi();
    void handleWiFiSave();
    void handleRoomName();
    void handleRoomNameSave();
    void handleSuccess();
    void handleCommand();
    void handleCss();
    void handleNotFound();
  private:
    ESP8266WebServer* server;
    bool shouldRestart;
};

#endif
