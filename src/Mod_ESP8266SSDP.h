/*
ESP8266 Simple Service Discovery
Copyright (c) 2015 Hristo Gochkov

Original (Arduino) version by Filippo Sallemi, July 23, 2014.
Can be found at: https://github.com/nomadnt/uSSDP

License (MIT license):
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.

*/

#ifndef ESP8266SSDP_H
#define ESP8266SSDP_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

class UdpContext;

#define SSDP_UUID_SIZE              42
#define SSDP_SCHEMA_URL_SIZE        64
#define SSDP_DEVICE_TYPE_SIZE       64
#define SSDP_FRIENDLY_NAME_SIZE     64
#define SSDP_SERIAL_NUMBER_SIZE     37
#define SSDP_PRESENTATION_URL_SIZE  128
#define SSDP_MODEL_NAME_SIZE        64
#define SSDP_MODEL_URL_SIZE         128
#define SSDP_MODEL_VERSION_SIZE     32
#define SSDP_MANUFACTURER_SIZE      64
#define SSDP_MANUFACTURER_URL_SIZE  128
#define SSDP_INTERVAL_SECONDS       1200
#define SSDP_MULTICAST_TTL          2
#define SSDP_HTTP_PORT              80

typedef enum {
  NONE,
  SEARCH,
  NOTIFY
} ssdp_method_t;


struct SSDPTimer;

class SSDPClass{
  public:
    SSDPClass();
    ~SSDPClass();
    bool begin();
    void end();
    void schema(WiFiClient client) const { schema((Print&)std::ref(client)); }
    void schema(Print &print) const;

    uint16_t port = SSDP_HTTP_PORT;
    uint8_t ttl = SSDP_MULTICAST_TTL;
    uint32_t interval = SSDP_INTERVAL_SECONDS;

    char schemaURL[SSDP_SCHEMA_URL_SIZE];
    char uuid[SSDP_UUID_SIZE];
    char deviceType[SSDP_DEVICE_TYPE_SIZE];
    char friendlyName[SSDP_FRIENDLY_NAME_SIZE];
    char serialNumber[SSDP_SERIAL_NUMBER_SIZE];
    char presentationURL[SSDP_PRESENTATION_URL_SIZE];
    char manufacturer[SSDP_MANUFACTURER_SIZE];
    char manufacturerURL[SSDP_MANUFACTURER_URL_SIZE];
    char modelName[SSDP_MODEL_NAME_SIZE];
    char modelURL[SSDP_MODEL_URL_SIZE];
    char modelNumber[SSDP_MODEL_VERSION_SIZE];

  protected:
    void _send(ssdp_method_t method);
    void _update();
    void _startTimer();
    void _stopTimer();
    static void _onTimerStatic(SSDPClass* self);

    UdpContext* _server = nullptr;
    SSDPTimer* _timer = nullptr;

    IPAddress _respondToAddr;
    uint16_t  _respondToPort = 0;

    bool _pending = false;
    bool _st_is_uuid = false;
    unsigned short _delay = 0;
    unsigned long _process_time = 0;
    unsigned long _notify_time = 0;
};

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_SSDP)
extern SSDPClass SSDP;
#endif

#endif
