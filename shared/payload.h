#pragma once
#include <stdint.h>

/*
 * Format de la payload LoRa — partagé entre le node et le gateway.
 *
 * Encodage big-endian pour la compatibilité entre architectures.
 * Les valeurs flottantes sont converties en entiers pour minimiser
 * la taille (pas de IEEE 754 sur 4 bytes pour chaque champ).
 *
 * byte 0-3 : device_id   — uint32 big-endian, 32 bits bas du MAC ESP32
 * byte 4-5 : température — int16 big-endian, °C × 100  (ex: 21.5°C → 2150)
 * byte 6-7 : pression    — uint16 big-endian, hPa entier (ex: 1013 hPa → 1013)
 * byte 8-9 : pm2.5       — uint16 big-endian, µg/m³ × 10 (ex: 12.5 → 125)
 *
 * Total : 10 bytes — largement sous la limite de 51 bytes LoRa SF9/125kHz
 */

#define PAYLOAD_LEN 10

// Structure interne côté C++ — les floats sont plus pratiques pour le calcul
struct SensorPayload {
    uint32_t device_id;
    float    temperature;  // °C
    float    pressure;     // hPa
    float    pm25;         // µg/m³
};

// Convertit la structure en tableau d'octets pour la transmission LoRa
inline void encodePayload(uint8_t *buf, const SensorPayload &p) {
    // Température signée (peut être négative) → int16
    int16_t  temp_raw = (int16_t)(p.temperature * 100.0f);
    // Pression toujours positive, troncature suffisante (précision 1 hPa)
    uint16_t pres_raw = (uint16_t)(p.pressure);
    // PM2.5 × 10 pour garder une décimale sans perdre de précision
    uint16_t pm25_raw = (uint16_t)(p.pm25 * 10.0f);

    buf[0] = (p.device_id >> 24) & 0xFF;
    buf[1] = (p.device_id >> 16) & 0xFF;
    buf[2] = (p.device_id >>  8) & 0xFF;
    buf[3] =  p.device_id        & 0xFF;
    buf[4] = (temp_raw    >>  8) & 0xFF;
    buf[5] =  temp_raw           & 0xFF;
    buf[6] = (pres_raw    >>  8) & 0xFF;
    buf[7] =  pres_raw           & 0xFF;
    buf[8] = (pm25_raw    >>  8) & 0xFF;
    buf[9] =  pm25_raw           & 0xFF;
}

// Reconstruit la structure à partir des octets reçus par LoRa
inline void decodePayload(const uint8_t *buf, SensorPayload &p) {
    p.device_id   = ((uint32_t)buf[0] << 24) | ((uint32_t)buf[1] << 16)
                  | ((uint32_t)buf[2] <<  8) |  (uint32_t)buf[3];
    // Cast en int16_t avant de diviser pour préserver le signe (températures négatives)
    int16_t temp  = ((int16_t)buf[4]  <<  8) | buf[5];
    p.temperature = temp / 100.0f;
    p.pressure    = (float)(((uint16_t)buf[6] << 8) | buf[7]);
    p.pm25        = (float)(((uint16_t)buf[8] << 8) | buf[9]) / 10.0f;
}
