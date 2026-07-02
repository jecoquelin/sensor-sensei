#pragma once

#include <Arduino.h>
#include "../../shared/payload.h"

bool portalInit();
void portalLoop();

void portalSetStationStatus(bool connected, const char *ssid, const IPAddress &ip,
                            const char *message);
void portalSetLoraStatus(bool ready, const char *message);
void portalSetLastPacket(const SensorPayload &p, float rssi, float snr);
