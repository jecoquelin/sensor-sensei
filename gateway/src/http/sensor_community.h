#pragma once
#include "../../shared/payload.h"

// Envoie les données BMP280 (pin 11), dust (pin 1) et micro (pin 15, DNMS) à sensor.community.
// Retourne true si les trois POST ont réussi.
bool scSend(const SensorPayload &p);
