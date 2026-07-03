#include "gatekeeper.h"
#include "settings/settings.h"
#include <Arduino.h>
#include <cstring>
#include <cstdio>

namespace {

uint32_t authorizedDevices[GATEWAY_MAX_WHITELIST];
size_t authorizedCount = 0;

}  // namespace

void gatekeeperInit() {
    authorizedCount = 0;
    memset(authorizedDevices, 0, sizeof(authorizedDevices));
}

void gatekeeperSetWhitelist(const uint32_t *devices, size_t count) {
    gatekeeperInit();
    if (devices == nullptr || count == 0) return;
    if (count > GATEWAY_MAX_WHITELIST) count = GATEWAY_MAX_WHITELIST;

    for (size_t i = 0; i < count; ++i) {
        authorizedDevices[i] = devices[i];
    }
    authorizedCount = count;
}

size_t gatekeeperGetWhitelist(uint32_t *devices, size_t maxDevices) {
    if (devices == nullptr || maxDevices == 0) return authorizedCount;
    size_t copyCount = authorizedCount < maxDevices ? authorizedCount : maxDevices;
    for (size_t i = 0; i < copyCount; ++i) {
        devices[i] = authorizedDevices[i];
    }
    return authorizedCount;
}

void gatekeeperFormatWhitelist(char *buffer, size_t bufferSize) {
    if (buffer == nullptr || bufferSize == 0) return;

    if (authorizedCount == 0) {
        strncpy(buffer, "accept all nodes", bufferSize - 1);
        buffer[bufferSize - 1] = '\0';
        return;
    }

    size_t used = 0;
    buffer[0] = '\0';
    for (size_t i = 0; i < authorizedCount; ++i) {
        int written = snprintf(buffer + used, bufferSize - used,
                               i == 0 ? "0x%08X" : ", 0x%08X",
                               authorizedDevices[i]);
        if (written < 0 || static_cast<size_t>(written) >= bufferSize - used) {
            buffer[bufferSize - 1] = '\0';
            return;
        }
        used += static_cast<size_t>(written);
    }
}

static bool isAuthorized(uint32_t device_id) {
    // Si la whitelist est vide, on accepte tout (mode dev)
    if (authorizedCount == 0) return true;

    for (size_t i = 0; i < authorizedCount; i++) {
        if (authorizedDevices[i] == device_id) return true;
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
