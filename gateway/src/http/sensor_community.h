#pragma once
#include "../../shared/payload.h"

// Envoie les données BMP280 (pin 3) et dust (pin 1) à sensor.community.
// Retourne true si les deux POST ont réussi.
bool scSend(const SensorPayload &p);
