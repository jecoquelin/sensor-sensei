#include "receiver.h"

#include <Arduino.h>
#include <RadioLib.h>
#include <SPI.h>

static SX1262 _radio = new Module(LORA_NSS, LORA_DIO1, LORA_RST, LORA_BUSY);

static volatile bool _rxDone = false;

void IRAM_ATTR _onReceive() {
    _rxDone = true;
}

bool loraInit() {
    SPI.begin(9, 11, 10, LORA_NSS);
    int state = _radio.begin(LORA_FREQ, LORA_BW, LORA_SF, LORA_CR, LORA_SYNC_WORD);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("[LoRa] Init failed: %d\n", state);
        return false;
    }
    _radio.setDio1Action(_onReceive);
    _radio.startReceive();
    Serial.println("[LoRa] Gateway ready - ecoute sur 868 MHz");
    return true;
}

bool loraReceive(LoRaFrame &frame) {
    if (!_rxDone) return false;
    _rxDone = false;

    uint8_t buf[LORA_PROTOCOL_FRAME_LEN];
    int state = _radio.readData(buf, sizeof(buf));

    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("[LoRa] RX error: %d\n", state);
        _radio.startReceive();
        return false;
    }

    LoraProtocolError error = loraProtocolDecodeFrame(buf, sizeof(buf), frame.sequence, frame.flags, frame.payload);
    if (error != LORA_PROTOCOL_OK) {
        Serial.printf("[LoRa] RX protocol error: %s\n", loraProtocolErrorString(error));
        _radio.startReceive();
        return false;
    }

    frame.version = buf[0];
    frame.rssi = _radio.getRSSI();
    frame.snr = _radio.getSNR();

    _radio.startReceive();
    return true;
}
