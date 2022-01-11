#ifndef CONFIG_H
#define CONFIG_H

#define AP_SSID "ESP8266 SimpleHome"
#define AP_PASSWORD "00000000"
#define DEFAULT_ROOM_NAME "Room"

#define LOGGING
#define LOGGING_INTERVAL 30000

#define LOOP_DELAY 100

#define SAVED_OR_DEFAULT_ROOM_NAME(string) (strlen(string) == 0 ? DEFAULT_ROOM_NAME : string)

#endif
