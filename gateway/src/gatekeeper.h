#pragma once
#include "../../shared/payload.h"
#include <stddef.h>

// ─── Whitelist des device_id autorisés ───────────────────────────────────────
// Ajouter ici les IDs des T-Beams enregistrés
static const uint16_t AUTHORIZED_DEVICES[] = {
    // 0x1234,
    // 0x5678,
};
static const size_t AUTHORIZED_DEVICES_COUNT =
    sizeof(AUTHORIZED_DEVICES) / sizeof(AUTHORIZED_DEVICES[0]);

// ─── Plages valides BMP280 ────────────────────────────────────────────────────
#define TEMP_MIN    -40.0f
#define TEMP_MAX     85.0f
#define PRES_MIN    300.0f
#define PRES_MAX   1100.0f

bool gatekeeperValidate(const SensorPayload &p);

