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
#ifndef LWIP_OPEN_SRC
#define LWIP_OPEN_SRC
#endif
#include <functional>
#include "Mod_ESP8266SSDP.h"
#include "WiFiUdp.h"
#include "debug.h"

extern "C" {
#include "osapi.h"
#include "ets_sys.h"
#include "user_interface.h"
}

#include "lwip/opt.h"
#include "lwip/udp.h"
#include "lwip/inet.h"
#include "lwip/igmp.h"
#include "lwip/mem.h"
#include "include/UdpContext.h"
//#define DEBUG_SSDP  Serial

#define SSDP_PORT         1900
#define SSDP_METHOD_SIZE  10
#define SSDP_URI_SIZE     2
#define SSDP_BUFFER_SIZE  64

// ssdp ipv6 is FF05::C
// lwip-v2's igmp_joingroup only supports IPv4
#define SSDP_MULTICAST_ADDR 239, 255, 255, 250

static const char _ssdp_response_template[] PROGMEM =
  "HTTP/1.1 200 OK\r\n"
  "EXT:\r\n";

static const char _ssdp_notify_template[] PROGMEM =
  "NOTIFY * HTTP/1.1\r\n"
  "HOST: 239.255.255.250:1900\r\n"
  "NTS: ssdp:alive\r\n";

static const char _ssdp_packet_template[] PROGMEM =
  "%s" // _ssdp_response_template / _ssdp_notify_template
  "CACHE-CONTROL: max-age=%u\r\n" // interval
  "SERVER: Arduino/1.0 UPNP/1.1 %s/%s\r\n" // modelName, modelNumber
  "USN: %s\r\n" // uuid
  "%s: %s\r\n"  // "NT" or "ST", deviceType
  "LOCATION: http://%s:%u/%s\r\n" // WiFi.localIP(), port, schemaURL
  "\r\n";

static const char _ssdp_schema_template[] PROGMEM =
  "HTTP/1.1 200 OK\r\n"
  "Content-Type: text/xml\r\n"
  "Connection: close\r\n"
  "Access-Control-Allow-Origin: *\r\n"
  "\r\n"
  "<?xml version=\"1.0\"?>"
  "<root xmlns=\"urn:schemas-upnp-org:device-1-0\">"
  "<specVersion>"
  "<major>1</major>"
  "<minor>0</minor>"
  "</specVersion>"
  "<URLBase>http://%s:%u/</URLBase>" // WiFi.localIP(), port
  "<device>"
  "<deviceType>%s</deviceType>"
  "<friendlyName>%s</friendlyName>"
  "<presentationURL>%s</presentationURL>"
  "<serialNumber>%s</serialNumber>"
  "<modelName>%s</modelName>"
  "<modelNumber>%s</modelNumber>"
  "<modelURL>%s</modelURL>"
  "<manufacturer>%s</manufacturer>"
  "<manufacturerURL>%s</manufacturerURL>"
  "<UDN>%s</UDN>"
  "</device>"
 //"<iconList>"
 //"<icon>"
 //"<mimetype>image/png</mimetype>"
 //"<height>48</height>"
 //"<width>48</width>"
 //"<depth>24</depth>"
 //"<url>icon48.png</url>"
 //"</icon>"
 //"<icon>"
 //"<mimetype>image/png</mimetype>"
 //"<height>120</height>"
 //"<width>120</width>"
 //"<depth>24</depth>"
 //"<url>icon120.png</url>"
 //"</icon>"
 //"</iconList>"
  "</root>\r\n"
  "\r\n";


struct SSDPTimer {
  ETSTimer timer;
};

SSDPClass::SSDPClass()
:  _respondToAddr(0,0,0,0)
{
  uuid[0] = '\0';
  modelNumber[0] = '\0';
  strcpy(deviceType, "urn:schemas-upnp-org:device:Basic:1");
  friendlyName[0] = '\0';
  presentationURL[0] = '\0';
  serialNumber[0] = '\0';
  modelName[0] = '\0';
  modelURL[0] = '\0';
  manufacturer[0] = '\0';
  manufacturerURL[0] = '\0';
  strcpy(schemaURL, "ssdp/schema.xml");
}

SSDPClass::~SSDPClass() {
  end();
}

bool SSDPClass::begin() {
  end();

  _pending = false;
  _st_is_uuid = false;
  if (strcmp(uuid,"") == 0) {
  	uint32_t chipId = ESP.getChipId();
  	sprintf_P(uuid, PSTR("uuid:38323636-4558-4dda-9188-cda0e6%02x%02x%02x"),
    (uint16_t) ((chipId >> 16) & 0xff),
  	(uint16_t) ((chipId >>  8) & 0xff),
  	(uint16_t)   chipId        & 0xff);
  }

#ifdef DEBUG_SSDP
  DEBUG_SSDP.printf("SSDP UUID: %s\n", (char *)uuid);
#endif

  assert(NULL == _server);

  _server = new UdpContext;
  _server->ref();

  IPAddress local = WiFi.localIP();
  IPAddress mcast(SSDP_MULTICAST_ADDR);

  if (igmp_joingroup(local, mcast) != ERR_OK ) {
#ifdef DEBUG_SSDP
    DEBUG_SSDP.printf_P(PSTR("SSDP failed to join igmp group\n"));
#endif
    return false;
  }

  if (!_server->listen(IP_ADDR_ANY, SSDP_PORT)) {
    return false;
  }

  _server->setMulticastInterface(local);
  _server->setMulticastTTL(ttl);
  _server->onRx(std::bind(&SSDPClass::_update, this));
  if (!_server->connect(mcast, SSDP_PORT)) {
    return false;
  }

  _startTimer();

  return true;
}

void SSDPClass::end() {
  if(!_server)
    return; // object is zeroed already, nothing to do

#ifdef DEBUG_SSDP
    DEBUG_SSDP.printf_P(PSTR("SSDP end ... "));
#endif
  // undo all initializations done in begin(), in reverse order
  _stopTimer();

  _server->disconnect();

  IPAddress local = WiFi.localIP();
  IPAddress mcast(SSDP_MULTICAST_ADDR);

  if (igmp_leavegroup(local, mcast) != ERR_OK ) {
#ifdef DEBUG_SSDP
    DEBUG_SSDP.printf_P(PSTR("SSDP failed to leave igmp group\n"));
#endif
  }

  _server->unref();
  _server = 0;

#ifdef DEBUG_SSDP
    DEBUG_SSDP.printf_P(PSTR("ok\n"));
#endif
}
void SSDPClass::_send(ssdp_method_t method) {
  char buffer[1460];
  IPAddress ip = WiFi.localIP();

  char valueBuffer[strlen_P(_ssdp_notify_template) + 1];
  strcpy_P(valueBuffer, (method == NONE) ? _ssdp_response_template : _ssdp_notify_template);

  int len = snprintf_P(buffer, sizeof(buffer),
                       _ssdp_packet_template,
                       valueBuffer,
                       interval,
                       modelName,
                       modelNumber,
                       uuid,
                       (method == NONE) ? "ST" : "NT",
                       (_st_is_uuid) ? uuid : deviceType,
                       ip.toString().c_str(), port, schemaURL
                      );

  _server->append(buffer, len);

  IPAddress remoteAddr;
  uint16_t remotePort;
  if (method == NONE) {
    remoteAddr = _respondToAddr;
    remotePort = _respondToPort;
#ifdef DEBUG_SSDP
    DEBUG_SSDP.print("Sending Response to ");
#endif
  } else {
    remoteAddr = IPAddress(SSDP_MULTICAST_ADDR);
    remotePort = SSDP_PORT;
#ifdef DEBUG_SSDP
    DEBUG_SSDP.println("Sending Notify to ");
#endif
  }
#ifdef DEBUG_SSDP
  DEBUG_SSDP.print(IPAddress(remoteAddr));
  DEBUG_SSDP.print(":");
  DEBUG_SSDP.println(remotePort);
#endif

  _server->send(remoteAddr, remotePort);
}

void SSDPClass::schema(Print &client) const {
  IPAddress ip = WiFi.localIP();
  char buffer[strlen_P(_ssdp_schema_template) + 1];
  strcpy_P(buffer, _ssdp_schema_template);
  client.printf(buffer,
                ip.toString().c_str(), port,
                deviceType,
                friendlyName,
                presentationURL,
                serialNumber,
                modelName,
                modelNumber,
                modelURL,
                manufacturer,
                manufacturerURL,
                uuid
               );
}

void SSDPClass::_update() {
  if (!_pending && _server->next()) {
    ssdp_method_t method = NONE;

    _respondToAddr = _server->getRemoteAddress();
    _respondToPort = _server->getRemotePort();

    typedef enum {METHOD, URI, PROTO, KEY, VALUE, ABORT} states;
    states state = METHOD;

    typedef enum {START, MAN, ST, MX} headers;
    headers header = START;

    uint8_t cursor = 0;
    uint8_t cr = 0;

    char buffer[SSDP_BUFFER_SIZE] = {0};

    while (_server->getSize() > 0) {
      char c = _server->read();

      cr = (c == '\r' || c == '\n') ? cr + 1 : 0;

      switch (state) {
        case METHOD:
          if (c == ' ') {
            if (strcmp(buffer, "M-SEARCH") == 0) method = SEARCH;

            state = method == NONE ? ABORT : URI;
            cursor = 0;

          } else if (cursor < SSDP_METHOD_SIZE - 1) {
            buffer[cursor++] = c;
            buffer[cursor] = '\0';
          }
          break;
        case URI:
          if (c == ' ') {
            state = strcmp(buffer, "*") ? ABORT : PROTO;
            cursor = 0;
          } else if (cursor < SSDP_URI_SIZE - 1) {
            buffer[cursor++] = c;
            buffer[cursor] = '\0';
          }
          break;
        case PROTO:
          if (cr == 2) {
            state = KEY;
            cursor = 0;
          }
          break;
        case KEY:
          if (cr == 4) {
            _pending = true;
            _process_time = millis();
          }
          else if (c == ' ') {
            cursor = 0;
            state = VALUE;
          }
          else if (c != '\r' && c != '\n' && c != ':' && cursor < SSDP_BUFFER_SIZE - 1) {
            buffer[cursor++] = c;
            buffer[cursor] = '\0';
          }
          break;
        case VALUE:
          if (cr == 2) {
            switch (header) {
              case START:
                break;
              case MAN:
#ifdef DEBUG_SSDP
                DEBUG_SSDP.printf("MAN: %s\n", (char *)buffer);
#endif
                break;
              case ST:
                if (strcmp(buffer, "ssdp:all")) {
                  state = ABORT;
#ifdef DEBUG_SSDP
                  DEBUG_SSDP.printf("REJECT: %s\n", (char *)buffer);
#endif
                }else{
                  _st_is_uuid = false;
                }
                // if the search type matches our type, we should respond instead of ABORT
                if (strcasecmp(buffer, deviceType) == 0) {
                  _pending = true;
                  _st_is_uuid = false;
                  _process_time = millis();
                  state = KEY;
                }
                if (strcasecmp(buffer, uuid) == 0) {
                  _pending = true;
                  _st_is_uuid = true;
                  _process_time = millis();
                  state = KEY;
                }
                break;
              case MX:
                _delay = random(0, atoi(buffer)) * 1000L;
                break;
            }

            if (state != ABORT) {
              state = KEY;
              header = START;
              cursor = 0;
            }
          } else if (c != '\r' && c != '\n') {
            if (header == START) {
              if (strncmp(buffer, "MA", 2) == 0) header = MAN;
              else if (strcmp(buffer, "ST") == 0) header = ST;
              else if (strcmp(buffer, "MX") == 0) header = MX;
            }

            if (cursor < SSDP_BUFFER_SIZE - 1) {
              buffer[cursor++] = c;
              buffer[cursor] = '\0';
            }
          }
          break;
        case ABORT:
          _pending = false; _delay = 0;
          break;
      }
    }
  }

  if (_pending && (millis() - _process_time) > _delay) {
    _pending = false; _delay = 0;
    _send(NONE);
  } else if(_notify_time == 0 || (millis() - _notify_time) > (interval * 1000L)){
    _notify_time = millis();
    _st_is_uuid = false;
    _send(NOTIFY);
  }

  if (_pending) {
    while (_server->next())
      _server->flush();
  }

}

void SSDPClass::_onTimerStatic(SSDPClass* self) {
  self->_update();
}

void SSDPClass::_startTimer() {
  _stopTimer();
  _timer = new SSDPTimer();
  ETSTimer* tm = &(_timer->timer);
  const int interval = 1000;
  os_timer_disarm(tm);
  os_timer_setfn(tm, reinterpret_cast<ETSTimerFunc*>(&SSDPClass::_onTimerStatic), reinterpret_cast<void*>(this));
  os_timer_arm(tm, interval, 1 /* repeat */);
}

void SSDPClass::_stopTimer() {
  if(!_timer)
    return;

  ETSTimer* tm = &(_timer->timer);
  os_timer_disarm(tm);
  delete _timer;
  _timer = NULL;
}

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_SSDP)
SSDPClass SSDP;
#endif
