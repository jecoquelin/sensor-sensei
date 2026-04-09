#include <Arduino.h>
#include <SPI.h>
#include <RadioLib.h>

/*
Branchement SX1262 (intégré Heltec V3 - interne)
┌────────┬────────┐
│ Signal │  GPIO  │
├────────┼────────┤
│ SCK    │ 9      │
│ MISO   │ 11     │
│ MOSI   │ 10     │
│ NSS    │ 8      │
│ RESET  │ 12     │
│ DIO1   │ 14     │
│ BUSY   │ 13     │
└────────┴────────┘

RSSI - Plus c'est proche de 0, meilleur c'est 
SNR - rapport signal/bruit, en dB
┌────────────┬────────────┬──────────┬────────────┐                                                                                                                                                       
│ Indicateur │    Bon     │  Limite  │    Mort    │                                                                                                                                                       
├────────────┼────────────┼──────────┼────────────┤                                                                                                                                                       
│ RSSI       │ > -100 dBm │ -110 dBm │ < -120 dBm │
├────────────┼────────────┼──────────┼────────────┤                                                                                                                                                       
│ SNR        │ > 0 dB     │ -10 dB   │ < -20 dB   │
└────────────┴────────────┴──────────┴────────────┘ 
*/

// ─── SX1262 pins (Heltec WiFi LoRa 32 V3) ────────────────────────────────────
#define LORA_NSS    8
#define LORA_DIO1   14
#define LORA_RST    12
#define LORA_BUSY   13

// ─── LoRa config — doit correspondre au node ─────────────────────────────────
#define LORA_FREQ       868.0
#define LORA_BW         125.0
#define LORA_SF         9
#define LORA_CR         7
#define LORA_SYNC_WORD  0x12

// ─── Payload layout (identique au node) ──────────────────────────────────────
// byte 0-1 : device id   (uint16, big-endian)
// byte 2-3 : température (int16, °C × 100)
// byte 4-5 : pression    (uint16, hPa)
#define PAYLOAD_LEN 6

SX1262 radio = new Module(LORA_NSS, LORA_DIO1, LORA_RST, LORA_BUSY);

volatile bool rxDone = false;

void IRAM_ATTR onReceive() {
    rxDone = true;
}

void decodePayload(uint8_t *buf, uint8_t len) {
    if (len < PAYLOAD_LEN) {
        Serial.printf("[RX] Payload trop court: %d bytes\n", len);
        return;
    }

    uint16_t device_id = ((uint16_t)buf[0] << 8) | buf[1];
    int16_t  temp_raw  = ((int16_t)buf[2]  << 8) | buf[3];
    uint16_t pres_raw  = ((uint16_t)buf[4] << 8) | buf[5];

    float temp     = temp_raw / 100.0f;
    float pressure = (float)pres_raw;

    Serial.println("─────────────────────────────");
    Serial.printf("Device ID  : 0x%04X\n", device_id);
    Serial.printf("Température: %.2f °C\n", temp);
    Serial.printf("Pression   : %.0f hPa\n", pressure);
    Serial.printf("RSSI       : %.1f dBm\n", radio.getRSSI());
    Serial.printf("SNR        : %.1f dB\n",  radio.getSNR());
    Serial.println("─────────────────────────────");
}

void setup() {
    Serial.begin(115200);

    SPI.begin(9, 11, 10, LORA_NSS);
    int state = radio.begin(LORA_FREQ, LORA_BW, LORA_SF, LORA_CR,
                            LORA_SYNC_WORD);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("[LoRa] Init failed: %d\n", state);
        while (1);
    }
    Serial.println("[LoRa] Gateway ready — écoute sur 868 MHz");

    radio.setDio1Action(onReceive);
    radio.startReceive();
}

void loop() {
    if (!rxDone) return;
    rxDone = false;

    uint8_t buf[PAYLOAD_LEN];
    int state = radio.readData(buf, PAYLOAD_LEN);

    if (state == RADIOLIB_ERR_NONE) {
        decodePayload(buf, PAYLOAD_LEN);
    } else {
        Serial.printf("[LoRa] RX error: %d\n", state);
    }

    // Relance l'écoute
    radio.startReceive();
}
