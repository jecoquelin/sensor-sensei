#pragma once
#include <stdint.h>

// ─── Payload layout (partagé node ↔ gateway) ─────────────────────────────────
// byte 0-3 : device id      (uint32, big-endian) — 32 bits bas du MAC ESP32
// byte 4-5 : température    (int16, °C × 100)
// byte 6-7 : pression       (uint16, hPa)
// byte 8-9 : pm2.5          (uint16, ug/m³ × 10)
// total: 10 bytes

#define PAYLOAD_LEN 10

struct SensorPayload {
    uint32_t device_id;
    float    temperature;  // °C
    float    pressure;     // hPa
    float    pm25;         // ug/m³
};

inline void encodePayload(uint8_t *buf, const SensorPayload &p) {
    int16_t  temp_raw = (int16_t)(p.temperature * 100.0f);
    uint16_t pres_raw = (uint16_t)(p.pressure);
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

inline void decodePayload(const uint8_t *buf, SensorPayload &p) {
    p.device_id   = ((uint32_t)buf[0] << 24) | ((uint32_t)buf[1] << 16)
                  | ((uint32_t)buf[2] <<  8) |  (uint32_t)buf[3];
    int16_t temp  = ((int16_t)buf[4]  <<  8) | buf[5];
    p.temperature = temp / 100.0f;
    p.pressure    = (float)(((uint16_t)buf[6] << 8) | buf[7]);
    p.pm25        = (float)(((uint16_t)buf[8] << 8) | buf[9]) / 10.0f;
}