#include "gatekeeper.h"
#include <Arduino.h>

static bool isAuthorized(uint32_t device_id) {
    // Si la whitelist est vide, on accepte tout (mode dev)
    // En prod : retirer ce guard et remplir AUTHORIZED_DEVICES
    if (AUTHORIZED_DEVICES_COUNT == 0) return true;

    for (size_t i = 0; i < AUTHORIZED_DEVICES_COUNT; i++) {
        if (AUTHORIZED_DEVICES[i] == device_id) return true;
    }
    return false;
}

static bool isInRange(const SensorPayload &p) {
    if (p.temperature < TEMP_MIN || p.temperature > TEMP_MAX) {
        Serial.printf("[GK] Température hors plage: %.2f °C\n", p.temperature);
        return false;
    }
    if (p.pressure < PRES_MIN || p.pressure > PRES_MAX) {
        Serial.printf("[GK] Pression hors plage: %.0f hPa\n", p.pressure);
        return false;
    }
    return true;
}

bool gatekeeperValidate(const SensorPayload &p) {
    if (!isAuthorized(p.device_id)) {
        Serial.printf("[GK] Device non autorisé: 0x%08X\n", p.device_id);
        return false;
    }
    if (!isInRange(p)) return false;

    Serial.printf("[GK] OK — Device 0x%08X validé\n", p.device_id);
    return true;
}
