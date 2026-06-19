#include <Wire.h>
#include <SPI.h>
#include <Adafruit_BMP280.h>
#include <RadioLib.h>
#include "sensors/dust.h"
#include "../../shared/payload.h"

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

Branchement Waveshare Dust Sensor
┌──────────────┬──────────────────────────────────────────────────┐
│ Dust Sensor  │                     T-Beam                       │
├──────────────┼──────────────────────────────────────────────────┤
│ VCC          │ 5V                                               │
├──────────────┼──────────────────────────────────────────────────┤
│ GND          │ GND                                              │
├──────────────┼──────────────────────────────────────────────────┤
│ LED          │ GPIO 2                                           │
├──────────────┼──────────────────────────────────────────────────┤
│ VOUT         │ GPIO 36 (ADC0/SVP) via pont diviseur 10k/20k     │
└──────────────┴──────────────────────────────────────────────────┘

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

// ─── Device ID — 32 bits bas du MAC, lu une seule fois au boot ───────────────
static uint32_t DEVICE_ID;

SX1276 radio = new Module(LORA_NSS, LORA_DIO0, LORA_RST, LORA_DIO1);
Adafruit_BMP280 bmp;
DustSensor dust(2, 36);

void setup() {
    Serial.begin(115200);
    DEVICE_ID = (uint32_t)(ESP.getEfuseMac() & 0xFFFFFFFF);
    Serial.printf("[NODE]   Device ID: 0x%08X\n", DEVICE_ID);

    // I2C for BMP280
    Wire.begin(21, 22);
    if (!bmp.begin(0x76)) {
        Serial.println("[BMP280] Not found !");
        while (1);
    }
    Serial.println("[BMP280] OK");

    // Dust sensor
    dust.begin();
    Serial.println("[DUST]   OK");

    // SX1276 SPI
    SPI.begin(5, 19, 27, LORA_NSS);
    int state = radio.begin(LORA_FREQ, LORA_BW, LORA_SF, LORA_CR,
                            LORA_SYNC_WORD, LORA_POWER);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("[LoRa]   Init failed: %d\n", state);
        while (1);
    }
    Serial.println("[LoRa]   OK");
}

void loop() {
    float temp     = bmp.readTemperature();
    float pressure = bmp.readPressure() / 100.0f;  // Pa → hPa
    float pm25     = dust.read();

    Serial.printf("[BMP280] Temp: %.2f °C | Pressure: %.2f hPa\n", temp, pressure);
    Serial.printf("[DUST]   PM2.5: %.1f ug/m3 | Voltage: %.0f mV\n",
    pm25, dust.getLastVoltage());

    SensorPayload p = { DEVICE_ID, temp, pressure, pm25 };
    uint8_t payload[PAYLOAD_LEN];
    encodePayload(payload, p);

    int state = radio.transmit(payload, sizeof(payload));
    if (state == RADIOLIB_ERR_NONE) {
        Serial.printf("[LoRa]   Sent %d bytes OK\n", sizeof(payload));
    } else {
        Serial.printf("[LoRa]   TX error: %d\n", state);
    }

    delay(10000);
}