#ifndef CONFIG_H
#define CONFIG_H

#define AP_SSID "ESP8266 SimpleHome"
#define AP_PASSWORD "00000000"
#define DEFAULT_ROOM_NAME "Room"

//#define LOGGING

#define LOOP_DELAY 200
#define PING_INTERVAL 60000

#define SAVED_OR_DEFAULT_ROOM_NAME(string) (string.length() == 0 ? DEFAULT_ROOM_NAME : string.c_str())

#endif
