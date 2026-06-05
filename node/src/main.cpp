#include <Wire.h>
#include <SPI.h>
#include <Adafruit_BMP280.h>
#include <RadioLib.h>

/*
Branchement BMP280 (HW-611)
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
*/

// ─── SX1276 pins (T-Beam v1.1) ───────────────────────────────────────────────
#define LORA_NSS    18
#define LORA_RST    23
#define LORA_DIO0   26
#define LORA_DIO1   33

// ─── LoRa config ─────────────────────────────────────────────────────────────
#define LORA_FREQ       868.0   // MHz
#define LORA_BW         125.0   // kHz
#define LORA_SF         9
#define LORA_CR         7       // 4/7
#define LORA_SYNC_WORD  0x12    // private network
#define LORA_POWER      14      // dBm

// ─── Device ID (2 bytes) ─────────────────────────────────────────────────────
#define DEVICE_ID  0x0001

// ─── Payload layout ──────────────────────────────────────────────────────────
// byte 0-1 : device id      (uint16, big-endian)
// byte 2-3 : temperature    (int16, °C × 100)
// byte 4-5 : pressure       (uint16, hPa)
// total: 6 bytes

SX1276 radio = new Module(LORA_NSS, LORA_DIO0, LORA_RST, LORA_DIO1);
Adafruit_BMP280 bmp;

void setup() {
    Serial.begin(115200);

    // I2C for BMP280
    Wire.begin(21, 22);
    if (!bmp.begin(0x76)) {
        Serial.println("[BMP280] Not found !");
        while (1);
    }
    Serial.println("[BMP280] OK");

    // SX1276 SPI
    SPI.begin(5, 19, 27, LORA_NSS);
    int state = radio.begin(LORA_FREQ, LORA_BW, LORA_SF, LORA_CR,
                            LORA_SYNC_WORD, LORA_POWER);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("[LoRa] Init failed: %d\n", state);
        while (1);
    }
    Serial.println("[LoRa] OK");
}

void buildPayload(uint8_t *buf, float temp, float pressure) {
    int16_t  temp_raw = (int16_t)(temp * 100.0f);
    uint16_t pres_raw = (uint16_t)(pressure);  // hPa, déjà divisé

    buf[0] = (DEVICE_ID >> 8) & 0xFF;
    buf[1] =  DEVICE_ID       & 0xFF;
    buf[2] = (temp_raw  >> 8) & 0xFF;
    buf[3] =  temp_raw        & 0xFF;
    buf[4] = (pres_raw  >> 8) & 0xFF;
    buf[5] =  pres_raw        & 0xFF;
}

void loop() {
    float temp     = bmp.readTemperature();
    float pressure = bmp.readPressure() / 100.0f;  // Pa → hPa

    Serial.printf("[BMP280] Temp: %.2f °C | Pressure: %.2f hPa\n", temp, pressure);

    uint8_t payload[6];
    buildPayload(payload, temp, pressure);

    int state = radio.transmit(payload, sizeof(payload));
    if (state == RADIOLIB_ERR_NONE) {
        Serial.printf("[LoRa] Sent %d bytes OK\n", sizeof(payload));
    } else {
        Serial.printf("[LoRa] TX error: %d\n", state);
    }

    delay(10000);
}
