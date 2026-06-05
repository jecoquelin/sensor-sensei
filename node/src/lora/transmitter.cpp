#include "transmitter.h"
#include <SPI.h>
#include <RadioLib.h>
#include <Arduino.h>

static SX1276 _radio = new Module(LORA_NSS, LORA_DIO0, LORA_RST, LORA_DIO1);

bool loraInit() {
    SPI.begin(5, 19, 27, LORA_NSS);
    int state = _radio.begin(LORA_FREQ, LORA_BW, LORA_SF, LORA_CR,
                             LORA_SYNC_WORD, LORA_POWER);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("[LoRa] Init failed: %d\n", state);
        return false;
    }
    Serial.println("[LoRa] OK");
    return true;
}

bool loraSend(const uint8_t *buf, size_t len) {
    int state = _radio.transmit((uint8_t *)buf, len);
    if (state == RADIOLIB_ERR_NONE) {
        Serial.printf("[LoRa] Sent %d bytes OK\n", len);
        return true;
    }
    Serial.printf("[LoRa] TX error: %d\n", state);
    return false;
}