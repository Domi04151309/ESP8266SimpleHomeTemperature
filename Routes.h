#ifndef ROUTES_H
#define ROUTES_H

#include <ESP8266WebServer.h>

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
    void handleNotFound();
  private:
    ESP8266WebServer* server;
    bool shouldRestart;
};

#endif
