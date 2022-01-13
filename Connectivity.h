#ifndef CONNECTIVITY_H
#define CONNECTIVITY_H

#include <cstdint>

void configureNetwork();
void startAP();
uint8_t RSSIToPercent(long rssi);

#endif
