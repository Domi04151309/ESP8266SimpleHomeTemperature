#ifndef CONNECTIVITY_H
#define CONNECTIVITY_H

#include <cstdint>

void configureNetwork();
void startAccessPoint();
uint8_t rssiToPercent(long rssi);

#endif
