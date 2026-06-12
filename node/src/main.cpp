#include <Wire.h>
#include <SPI.h>
// #include <Adafruit_BMP280.h>
#include <RadioLib.h>
#include "sensors/sph0645.h"

/*
Branchement BMP280 (HW-611) — désactivé
┌────────┬────────────────────┐
│ HW-611 │       T-Beam       │
├────────┼────────────────────┤
│ VCC    │ 3.3V               │
├────────┼────────────────────┤
│ GND    │ GND                │
├────────┼────────────────────┤
│ SDA    │ GPIO 21            │
├────────┼────────────────────┤
│ SCL    │ GPIO 22            │
├────────┼────────────────────┤
│ CSB    │ 3.3V ← critique    │
├────────┼────────────────────┤
│ SDD    │ GND (adresse 0x76) │
└────────┴────────────────────┘

Branchement SX1276 (intégré T-Beam)
┌────────┬────────┐
│ Signal │  GPIO  │
├────────┼────────┤
│ SCK    │ 5      │
│ MISO   │ 19     │
│ MOSI   │ 27     │
│ NSS    │ 18     │
│ RESET  │ 23     │
│ DIO0   │ 26     │
│ DIO1   │ 33     │
└────────┴────────┘

Branchement SPH0645 (micro I2S)
┌────────────┬────────────────────┐
│  SPH0645   │       T-Beam       │
├────────────┼────────────────────┤
│ 3V         │ 3.3V               │
├────────────┼────────────────────┤
│ GND        │ GND                │
├────────────┼────────────────────┤
│ BCLK       │ GPIO 32            │
├────────────┼────────────────────┤
│ LRCL (WS)  │ GPIO 25            │
├────────────┼────────────────────┤
│ DOUT       │ GPIO 34            │
├────────────┼────────────────────┤
│ SEL        │ GND                │
└────────────┴────────────────────┘
*/

#define LORA_NSS        18
#define LORA_RST        23
#define LORA_DIO0       26
#define LORA_DIO1       33

#define LORA_FREQ       868.0
#define LORA_BW         125.0
#define LORA_SF         9
#define LORA_CR         7
#define LORA_SYNC_WORD  0x12
#define LORA_POWER      14

#define DEVICE_ID       0x0001

// ─── Payload layout ──────────────────────────────────────────────────────────
// byte 0-1 : device id  (uint16, big-endian)
// byte 2-3 : mic RMS    (uint16, valeur × 10000)
// total: 4 bytes

SX1276 radio = new Module(LORA_NSS, LORA_DIO0, LORA_RST, LORA_DIO1);
// Adafruit_BMP280 bmp;

void setup() {
    Serial.begin(115200);

    // Wire.begin(21, 22);
    // if (!bmp.begin(0x76)) {
    //     Serial.println("[BMP280] Not found !");
    //     while (1);
    // }
    // Serial.println("[BMP280] OK");

    if (!micInit()) {
        Serial.println("[MIC] Init failed !");
        while (1);
    }

    SPI.begin(5, 19, 27, LORA_NSS);
    int state = radio.begin(LORA_FREQ, LORA_BW, LORA_SF, LORA_CR,
                            LORA_SYNC_WORD, LORA_POWER);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("[LoRa] Init failed: %d\n", state);
        while (1);
    }
    Serial.println("[LoRa] OK");
}

void buildPayload(uint8_t *buf, float rms) {
    uint16_t mic_raw = (uint16_t)(rms * 10000.0f);

    buf[0] = (DEVICE_ID >> 8) & 0xFF;
    buf[1] =  DEVICE_ID       & 0xFF;
    buf[2] = (mic_raw   >> 8) & 0xFF;
    buf[3] =  mic_raw         & 0xFF;
}

void loop() {
    static uint32_t last_send = 0;
    static float    rms_sum   = 0.0f;
    static uint32_t rms_count = 0;

    // Lecture continue du micro
    float rms = micReadRMS();
    if (rms > 0.001f) {
        rms_sum += rms;
        rms_count++;
    }

    if (millis() - last_send >= 10000) {
        float rms_avg = (rms_count > 0) ? rms_sum / rms_count : 0.0f;

        Serial.printf("[MIC] RMS avg: %.4f | %.1f dBFS (sur %d lectures)\n",
    rms_avg, 20.0f * log10f(rms_avg > 0.0f ? rms_avg : 1e-9f), rms_count);

        uint8_t payload[4];
        buildPayload(payload, rms_avg);

        int state = radio.transmit(payload, sizeof(payload));
        if (state == RADIOLIB_ERR_NONE) {
            Serial.printf("[LoRa] Sent %d bytes OK\n", sizeof(payload));
        } else {
            Serial.printf("[LoRa] TX error: %d\n", state);
        }

        // Reset pour la prochaine fenêtre
        rms_sum   = 0.0f;
        rms_count = 0;
        last_send = millis();
    }
}