#pragma once
#include "../../shared/payload.h"
#include <stddef.h>

// ─── Plages valides BMP280 ────────────────────────────────────────────────────
#define TEMP_MIN    -40.0f
#define TEMP_MAX     85.0f
#define PRES_MIN    300.0f
#define PRES_MAX   1100.0f

void gatekeeperInit();
void gatekeeperSetWhitelist(const uint32_t *devices, size_t count);
size_t gatekeeperGetWhitelist(uint32_t *devices, size_t maxDevices);
void gatekeeperFormatWhitelist(char *buffer, size_t bufferSize);
bool gatekeeperValidate(const SensorPayload &p);
