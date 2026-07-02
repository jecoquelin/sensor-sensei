#pragma once

#include "../../shared/payload.h"

void httpOutboxInit();
void httpOutboxEnqueueMeasurement(const SensorPayload &payload);
void httpOutboxTick();
uint16_t httpOutboxPendingCount();
