/**
 * ESP8266HTTPClient.cpp
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
 */
#include <Arduino.h>

#include "Mod_ESP8266HTTPClient.h"
#include <ESP8266WiFi.h>
#include <StreamDev.h>
#include <base64.h>

static int StreamReportToHttpClientReport (Stream::Report streamSendError)
{
    switch (streamSendError)
    {
    case Stream::Report::TimedOut: return HTTPC_ERROR_READ_TIMEOUT;
    case Stream::Report::ReadError: return HTTPC_ERROR_NO_STREAM;
    case Stream::Report::WriteError: return HTTPC_ERROR_STREAM_WRITE;
    case Stream::Report::ShortOperation: return HTTPC_ERROR_STREAM_WRITE;
    case Stream::Report::Success: return 0;
    }
    return 0; // never reached, keep gcc quiet
}

/**
 * constructor
 */
HTTPClient::HTTPClient()
    : _client(nullptr), _userAgent(F("ESP8266HTTPClient"))
{
}

/**
 * destructor
 */
HTTPClient::~HTTPClient()
{
    if(_client) {
        _client->stop();
    }
    if(_currentHeaders) {
        delete[] _currentHeaders;
    }
}

void HTTPClient::clear()
{
    _returnCode = 0;
    _size = -1;
    _payload.reset();
}


/**
 * directly supply all needed parameters
 * @param client Client&
 * @param host String
 * @param port uint16_t
 * @param uri String
 * @param https bool
 * @return success bool
 */
bool HTTPClient::begin(WiFiClient &client, const String& host, uint16_t port, const String& uri, bool https)
{
    _client = &client;

     clear();
    _host = host;
    _port = port;
    _uri = uri;
    _protocol = (https ? "https" : "http");
    return true;
}

/**
 * end
 * called after the payload is handled
 */
void HTTPClient::end(void)
{
    disconnect(false);
    clear();
}

/**
 * disconnect
 * close the TCP socket
 */
void HTTPClient::disconnect(bool preserveClient)
{
    if(connected()) {
        if(_client->available() > 0) {
            DEBUG_HTTPCLIENT("[HTTP-Client][end] still data in buffer (%d), clean up.\n", _client->available());
            while(_client->available() > 0) {
                _client->read();
            }
        }

        DEBUG_HTTPCLIENT("[HTTP-Client][end] tcp stop\n");
        if(_client) {
            _client->stop();
            if (!preserveClient) {
                _client = nullptr;
            }
        }
    } else {
        if (!preserveClient && _client) { // Also destroy _client if not connected()
            _client = nullptr;
        }

        DEBUG_HTTPCLIENT("[HTTP-Client][end] tcp is closed\n");
    }
}

/**
 * connected
 * @return connected status
 */
bool HTTPClient::connected()
{
    if(_client) {
        return (_client->connected() || (_client->available() > 0));
    }
    return false;
}

/**
 * send a GET request
 * @return http code
 */
int HTTPClient::GET()
{
    return sendRequest("GET");
}

/**
 * sendRequest
 * @param type const char *           "GET", "POST", ....
 * @return -1 if no info or > 0 when Content-Length is set by server
 */
int HTTPClient::sendRequest(const char * type)
{
    int code;

    // wipe out any existing headers from previous request
    for(size_t i = 0; i < _headerKeysCount; i++) {
        if (_currentHeaders[i].value.length() > 0) {
            _currentHeaders[i].value.clear();
        }
    }

    DEBUG_HTTPCLIENT("[HTTP-Client][sendRequest] type: '%s'\n", type);

    // connect to server
    if(!connect()) {
        return returnError(HTTPC_ERROR_CONNECTION_FAILED);
    }

    // send Header
    if(!sendHeader(type)) {
        return returnError(HTTPC_ERROR_SEND_HEADER_FAILED);
    }

    // handle Server Response (Header)
    code = handleHeaderResponse();

    // handle Server Response (Header)
    return returnError(code);
}

/**
 * write all  message body / payload to Stream
 * @param stream Stream *
 * @return bytes written ( negative values are error codes )
 */
int HTTPClient::writeToStream(Stream * stream)
{

    if(!stream) {
        return returnError(HTTPC_ERROR_NO_STREAM);
    }

    // Only return error if not connected and no data available, because otherwise ::getString() will return an error instead of an empty
    // string when the server returned a http code 204 (no content)
    if(!connected() && _transferEncoding != HTTPC_TE_IDENTITY && _size > 0) {
        return returnError(HTTPC_ERROR_NOT_CONNECTED);
    }

    // get length of document (is -1 when Server sends no Content-Length header)
    int len = _size;
    int ret = 0;

    if(_transferEncoding == HTTPC_TE_IDENTITY) {
        // len < 0: transfer all of it, with timeout
        // len >= 0: max:len, with timeout
        ret = _client->sendSize(stream, len);

        // do we have an error?
        if(_client->getLastSendReport() != Stream::Report::Success) {
            return returnError(StreamReportToHttpClientReport(_client->getLastSendReport()));
        }
    } else if(_transferEncoding == HTTPC_TE_CHUNKED) {
        int size = 0;
        while(1) {
            if(!connected()) {
                return returnError(HTTPC_ERROR_CONNECTION_LOST);
            }
            String chunkHeader = _client->readStringUntil('\n');

            if(chunkHeader.length() <= 0) {
                return returnError(HTTPC_ERROR_READ_TIMEOUT);
            }

            chunkHeader.trim(); // remove \r

            // read size of chunk
            len = (uint32_t) strtol((const char *) chunkHeader.c_str(), NULL, 16);
            size += len;
            DEBUG_HTTPCLIENT("[HTTP-Client] read chunk len: %d\n", len);

            // data left?
            if(len > 0) {
                // read len bytes with timeout
                int r = _client->sendSize(stream, len);
                if (_client->getLastSendReport() != Stream::Report::Success)
                    // not all data transferred
                    return returnError(StreamReportToHttpClientReport(_client->getLastSendReport()));
                ret += r;
            } else {

                // if no length Header use global chunk size
                if(_size <= 0) {
                    _size = size;
                }

                // check if we have write all data out
                if(ret != _size) {
                    return returnError(HTTPC_ERROR_STREAM_WRITE);
                }
                break;
            }

            // read trailing \r\n at the end of the chunk
            char buf[2];
            auto trailing_seq_len = _client->readBytes((uint8_t*)buf, 2);
            if (trailing_seq_len != 2 || buf[0] != '\r' || buf[1] != '\n') {
                return returnError(HTTPC_ERROR_READ_TIMEOUT);
            }

            delay(0);
        }
    } else {
        return returnError(HTTPC_ERROR_ENCODING);
    }

    disconnect(true);
    return ret;
}

/**
 * return all payload as String (may need lot of ram or trigger out of memory!)
 * @return String
 */
const String& HTTPClient::getString(void)
{
    if (_payload) {
        return *_payload;
    }

    _payload.reset(new StreamString());

    if(_size > 0) {
        // try to reserve needed memory
        if(!_payload->reserve((_size + 1))) {
            DEBUG_HTTPCLIENT("[HTTP-Client][getString] not enough memory to reserve a string! need: %d\n", (_size + 1));
            return *_payload;
        }
    }

    writeToStream(_payload.get());
    return *_payload;
}

/**
 * init TCP connection and handle ssl verify if needed
 * @return true if connection is ok
 */
bool HTTPClient::connect(void)
{
    if(!_client) {
        DEBUG_HTTPCLIENT("[HTTP-Client] connect: HTTPClient::begin was not called or returned error\n");
        return false;
    }

    _client->setTimeout(HTTPCLIENT_DEFAULT_TCP_TIMEOUT);

    if(!_client->connect(_host.c_str(), _port)) {
        DEBUG_HTTPCLIENT("[HTTP-Client] failed connect to %s:%u\n", _host.c_str(), _port);
        return false;
    }

    DEBUG_HTTPCLIENT("[HTTP-Client] connected to %s:%u\n", _host.c_str(), _port);

#ifdef ESP8266
    _client->setNoDelay(true);
#endif
    return connected();
}

/**
 * sends HTTP request header
 * @param type (GET, POST, ...)
 * @return status
 */
bool HTTPClient::sendHeader(const char * type)
{
    if(!connected()) {
        return false;
    }

    String header;
    // 128: Arbitrarily chosen to have enough buffer space for avoiding internal reallocations
    header.reserve(_uri.length() + _host.length() + _userAgent.length() + 128);
    header += type;
    header += ' ';
    if (_uri.length()) {
        header += _uri;
    } else {
        header += '/';
    }
    header += F(
      " HTTP/1.1"
      "\r\nHost: "
    );
    header += _host;
    if (_port != 80 && _port != 443)
    {
        header += ':';
        header += String(_port);
    }
    header += F("\r\nUser-Agent: ");
    header += _userAgent;

    header += F(
      "\r\nAccept-Encoding: identity;q=1,chunked;q=0.1,*;q=0"
      "\r\nConnection: close"
      "\r\n"
      "\r\n"
    );

    DEBUG_HTTPCLIENT("[HTTP-Client] sending request header\n-----\n%s-----\n", header.c_str());

    // transfer all of it, with timeout
    return StreamConstPtr(header).sendAll(_client) == header.length();
}

/**
 * reads the response from the server
 * @return int http code
 */
int HTTPClient::handleHeaderResponse()
{

    if(!connected()) {
        return HTTPC_ERROR_NOT_CONNECTED;
    }

    clear();

    String transferEncoding;

    _transferEncoding = HTTPC_TE_IDENTITY;
    unsigned long lastDataTime = millis();

    while(connected()) {
        size_t len = _client->available();
        if(len > 0) {
            int headerSeparator = -1;
            String headerLine = _client->readStringUntil('\n');

            lastDataTime = millis();

            DEBUG_HTTPCLIENT("[HTTP-Client][handleHeaderResponse] RX: '%s'\n", headerLine.c_str());

            if (headerLine.startsWith(F("HTTP/1."))) {
                constexpr auto httpVersionIdx = sizeof "HTTP/1." - 1;
                _returnCode = headerLine.substring(httpVersionIdx + 2, headerLine.indexOf(' ', httpVersionIdx + 2)).toInt();
            } else if ((headerSeparator = headerLine.indexOf(':')) > 0) {
                String headerName = headerLine.substring(0, headerSeparator);
                String headerValue = headerLine.substring(headerSeparator + 1);
                headerValue.trim();

                if(headerName.equalsIgnoreCase(F("Content-Length"))) {
                    _size = headerValue.toInt();
                }

                if(headerName.equalsIgnoreCase(F("Transfer-Encoding"))) {
                    transferEncoding = headerValue;
                }

                for (size_t i = 0; i < _headerKeysCount; i++) {
                    if (_currentHeaders[i].key.equalsIgnoreCase(headerName)) {
                        if (!_currentHeaders[i].value.isEmpty()) {
                            // Existing value, append this one with a comma
                            _currentHeaders[i].value += ',';
                            _currentHeaders[i].value += headerValue;
                        } else {
                            _currentHeaders[i].value = headerValue;
                        }
                        break; // We found a match, stop looking
                    }
                }
                continue;
            }

            headerLine.trim(); // remove \r

            if (headerLine.isEmpty()) {
                DEBUG_HTTPCLIENT("[HTTP-Client][handleHeaderResponse] code: %d\n", _returnCode);

                if(_size > 0) {
                    DEBUG_HTTPCLIENT("[HTTP-Client][handleHeaderResponse] size: %d\n", _size);
                }

                if(transferEncoding.length() > 0) {
                    DEBUG_HTTPCLIENT("[HTTP-Client][handleHeaderResponse] Transfer-Encoding: %s\n", transferEncoding.c_str());
                    if(transferEncoding.equalsIgnoreCase(F("chunked"))) {
                        _transferEncoding = HTTPC_TE_CHUNKED;
                    } else {
                        _returnCode = HTTPC_ERROR_ENCODING;
                        return _returnCode;
                    }
                } else {
                    _transferEncoding = HTTPC_TE_IDENTITY;
                }

                if(_returnCode <= 0) {
                    DEBUG_HTTPCLIENT("[HTTP-Client][handleHeaderResponse] Remote host is not an HTTP Server!");
                    _returnCode = HTTPC_ERROR_NO_HTTP_SERVER;
                }
                return _returnCode;
            }

        } else {
            if((millis() - lastDataTime) > HTTPCLIENT_DEFAULT_TCP_TIMEOUT) {
                return HTTPC_ERROR_READ_TIMEOUT;
            }
            delay(0);
        }
    }

    return HTTPC_ERROR_CONNECTION_LOST;
}

/**
 * called to handle error return, may disconnect the connection if still exists
 * @param error
 * @return error
 */
int HTTPClient::returnError(int error)
{
    if(error < 0) {
        DEBUG_HTTPCLIENT("[HTTP-Client][returnError] error(%d): %s\n", error, errorToString(error).c_str());
        if(connected()) {
            DEBUG_HTTPCLIENT("[HTTP-Client][returnError] tcp stop\n");
            _client->stop();
        }
    }
    return error;
}
