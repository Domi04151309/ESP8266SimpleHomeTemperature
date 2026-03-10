#ifndef CONNECTIVITY_H
#define CONNECTIVITY_H

#include <cstdint>
#include <WiFiUdp.h>

void configureNetwork();
void sendHeartbeat(WiFiUDP &udpInstance);
void startAccessPoint();
uint8_t rssiToPercent(long rssi);

#endif
