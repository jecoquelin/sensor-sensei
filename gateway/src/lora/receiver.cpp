#include "receiver.h"
#include <SPI.h>
#include <RadioLib.h>
#include <Arduino.h>

static SX1262 _radio = new Module(LORA_NSS, LORA_DIO1, LORA_RST, LORA_BUSY);

// Drapeau positionné par l'ISR — volatile car modifié hors du contexte principal
static volatile bool _rxDone = false;

// Routine d'interruption déclenchée par DIO1 du SX1262 à chaque paquet reçu.
// IRAM_ATTR force le placement en RAM interne pour une exécution rapide
// (le code en flash ne peut pas être exécuté pendant certaines opérations SPI).
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
    // L'interruption sur DIO1 permet une réception non-bloquante :
    // le gateway peut continuer à tourner sans attendre busy-poll le radio.
    _radio.setDio1Action(_onReceive);
    _radio.startReceive();
    Serial.println("[LoRa] Gateway ready — écoute sur 868 MHz");
    return true;
}

bool loraReceive(LoRaFrame &frame) {
    if (!_rxDone) return false;
    _rxDone = false;  // acquitte le flag avant readData() pour ne pas manquer un paquet suivant

    uint8_t buf[PAYLOAD_LEN];
    int state = _radio.readData(buf, PAYLOAD_LEN);

    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("[LoRa] RX error: %d\n", state);
        _radio.startReceive();  // remet le radio en écoute même en cas d'erreur
        return false;
    }

    decodePayload(buf, frame.payload);
    frame.rssi = _radio.getRSSI();
    frame.snr  = _radio.getSNR();

    // Relance l'écoute immédiatement — le SX1262 repasse en mode RX continu
    _radio.startReceive();
    return true;
}
