/**
 * ESP8266HTTPClient.h
 *
 * Created on: 02.11.2015
 *
 * Copyright (c) 2015 Markus Sattler. All rights reserved.
 * This file is part of the ESP8266HTTPClient for Arduino.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Modified by Jeroen Döll, June 2018
 */

#ifndef ESP8266HTTPClient_H_
#define ESP8266HTTPClient_H_

#include <memory>
#include <Arduino.h>
#include <StreamString.h>
#include <WiFiClient.h>

#ifdef DEBUG_ESP_HTTP_CLIENT
#ifdef DEBUG_ESP_PORT
#define DEBUG_HTTPCLIENT(fmt, ...) DEBUG_ESP_PORT.printf_P( (PGM_P)PSTR(fmt), ## __VA_ARGS__ )
#endif
#endif

#ifndef DEBUG_HTTPCLIENT
#define DEBUG_HTTPCLIENT(...) do { (void)0; } while (0)
#endif

#define HTTPCLIENT_DEFAULT_TCP_TIMEOUT (5000)

/// HTTP client errors
#define HTTPC_ERROR_CONNECTION_FAILED   (-1)
#define HTTPC_ERROR_SEND_HEADER_FAILED  (-2)
#define HTTPC_ERROR_SEND_PAYLOAD_FAILED (-3)
#define HTTPC_ERROR_NOT_CONNECTED       (-4)
#define HTTPC_ERROR_CONNECTION_LOST     (-5)
#define HTTPC_ERROR_NO_STREAM           (-6)
#define HTTPC_ERROR_NO_HTTP_SERVER      (-7)
#define HTTPC_ERROR_TOO_LESS_RAM        (-8)
#define HTTPC_ERROR_ENCODING            (-9)
#define HTTPC_ERROR_STREAM_WRITE        (-10)
#define HTTPC_ERROR_READ_TIMEOUT        (-11)

constexpr int HTTPC_ERROR_CONNECTION_REFUSED __attribute__((deprecated)) = HTTPC_ERROR_CONNECTION_FAILED;

/// size for the stream handling
#define HTTP_TCP_BUFFER_SIZE (1460)

typedef enum {
    HTTPC_TE_IDENTITY,
    HTTPC_TE_CHUNKED
} transferEncoding_t;

class TransportTraits;
typedef std::unique_ptr<TransportTraits> TransportTraitsPtr;

class HTTPClient
{
public:
    HTTPClient();
    ~HTTPClient();

/*
 * Since both begin() functions take a reference to client as a parameter, you need to
 * ensure the client object lives the entire time of the HTTPClient
 */
    bool begin(WiFiClient &client, const String& host, uint16_t port, const String& uri, bool https);
    void end(void);
    bool connected(void);

    /// request handling
    int GET();
    int sendRequest(const char* type);

    int writeToStream(Stream* stream);
    const String& getString(void);

protected:
    struct RequestArgument {
        String key;
        String value;
    };

    void disconnect(bool preserveClient = false);
    void clear();
    int returnError(int error);
    bool connect(void);
    bool sendHeader(const char * type);
    int handleHeaderResponse();

    WiFiClient* _client;

    /// request handling
    String _host;
    uint16_t _port = 0;

    String _uri;
    String _protocol;
    String _userAgent;

    /// Response handling
    RequestArgument* _currentHeaders = nullptr;
    size_t           _headerKeysCount = 0;

    int _returnCode = 0;
    int _size = -1;
    transferEncoding_t _transferEncoding = HTTPC_TE_IDENTITY;
    std::unique_ptr<StreamString> _payload;
};

#endif /* ESP8266HTTPClient_H_ */
