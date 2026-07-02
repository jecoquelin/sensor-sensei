#pragma once

#include "../../shared/payload.h"

struct SensorCommunityJob {
    uint32_t device_id;
    uint8_t pin;
    int16_t temperature_centi;
    uint16_t pressure_hpa;
    uint16_t pm25_deci;
};

bool scSendJob(const SensorCommunityJob &job);
bool scSend(const SensorPayload &p);
