#pragma once
#include <stdint.h>

// ─── Payload layout (partagé node ↔ gateway) ─────────────────────────────────
// byte 0-1 : device id      (uint16, big-endian)
// byte 2-3 : température    (int16, °C × 100)
// byte 4-5 : pression       (uint16, hPa)
// byte 6-7 : bruit          (int16, dB × 10)
// total: 8 bytes

#define PAYLOAD_LEN 8

struct SensorPayload {
    uint16_t device_id;
    float    temperature;  // °C
    float    pressure;     // hPa
    float    noise_db;     // dB SPL
};

inline void encodePayload(uint8_t *buf, const SensorPayload &p) {
    int16_t  temp_raw  = (int16_t)(p.temperature * 100.0f);
    uint16_t pres_raw  = (uint16_t)(p.pressure);
    int16_t  noise_raw = (int16_t)(p.noise_db * 10.0f);

    buf[0] = (p.device_id >> 8) & 0xFF;
    buf[1] =  p.device_id       & 0xFF;
    buf[2] = (temp_raw   >> 8)  & 0xFF;
    buf[3] =  temp_raw          & 0xFF;
    buf[4] = (pres_raw   >> 8)  & 0xFF;
    buf[5] =  pres_raw          & 0xFF;
    buf[6] = (noise_raw  >> 8)  & 0xFF;
    buf[7] =  noise_raw         & 0xFF;
}

inline void decodePayload(const uint8_t *buf, SensorPayload &p) {
    p.device_id   = ((uint16_t)buf[0] << 8) | buf[1];
    int16_t temp  = ((int16_t)buf[2]  << 8) | buf[3];
    int16_t noise = ((int16_t)buf[6]  << 8) | buf[7];
    p.temperature = temp  / 100.0f;
    p.pressure    = (float)(((uint16_t)buf[4] << 8) | buf[5]);
    p.noise_db    = noise / 10.0f;
}
