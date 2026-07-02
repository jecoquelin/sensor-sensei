/*
 * SensorSensei - Gateway (Heltec WiFi LoRa 32 V3)
 *
 * Le gateway ecoute en permanence sur 868 MHz. A chaque paquet recu :
 *   1. Verifie que l'expediteur est autorise (gatekeeper)
 *   2. Valide la trame LoRa (version, seq, flags, CRC16)
 *   3. Affiche les donnees sur l'ecran OLED
 *   4. Envoie les donnees a l'API sensor.community via HTTPS
 *
 * Il n'y a pas d'ACK applicatif: la robustesse repose sur la CRC, le seq et le
 * jitter avant emission pour limiter les collisions.
 */

#include <Arduino.h>
#include <WiFi.h>

#include "config.h"
#include "display/display.h"
#include "gatekeeper.h"
#include "http/sensor_community.h"
#include "lora/receiver.h"

struct SequenceState {
    bool used;
    uint32_t deviceId;
    uint8_t lastSeq;
};

static SequenceState sequenceStates[8] = {};

static void wifiConnect() {
    if (WiFi.status() == WL_CONNECTED) return;

    Serial.printf("[WiFi] Connexion a %s...\n", WIFI_SSID);
    displayStatus("Connexion WiFi...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 15000)
        delay(500);

    if (WiFi.status() == WL_CONNECTED)
        Serial.printf("[WiFi] Connecte - IP: %s\n", WiFi.localIP().toString().c_str());
    else
        Serial.println("[WiFi] Timeout - donnees non envoyees ce cycle");
}

static void recordSequence(uint32_t deviceId, uint8_t seq) {
    SequenceState *freeSlot = nullptr;

    for (SequenceState &state : sequenceStates) {
        if (!state.used) {
            if (!freeSlot) freeSlot = &state;
            continue;
        }

        if (state.deviceId != deviceId)
            continue;

        uint8_t expected = (uint8_t)(state.lastSeq + 1);
        if (seq != expected) {
            uint8_t missing = (uint8_t)(seq - expected);
            Serial.printf("[LoRa] Seq gap device 0x%08X: last=%u now=%u missing=%u\n",
                          deviceId, state.lastSeq, seq, missing);
        } else {
            Serial.printf("[LoRa] Seq OK device 0x%08X: %u\n", deviceId, seq);
        }

        state.lastSeq = seq;
        return;
    }

    if (!freeSlot) freeSlot = &sequenceStates[0];
    freeSlot->used = true;
    freeSlot->deviceId = deviceId;
    freeSlot->lastSeq = seq;
    Serial.printf("[LoRa] First seq device 0x%08X: %u\n", deviceId, seq);
}

void setup() {
    Serial.begin(115200);

    displayInit();
    wifiConnect();

    if (!loraInit()) {
        displayError("LoRa init failed");
        while (1);
    }
    displayStatus("Ecoute 868 MHz...");
}

void loop() {
    LoRaFrame frame;

    if (!loraReceive(frame))
        return;

    if (frame.version != LORA_PROTOCOL_VERSION) {
        Serial.printf("[LoRa] Version inattendue: %u\n", frame.version);
        return;
    }

    if (frame.flags != LORA_PROTOCOL_FLAG_NONE) {
        Serial.printf("[LoRa] Flags actifs: 0x%02X\n", frame.flags);
    }

    const SensorPayload &p = frame.payload;

    if (!gatekeeperValidate(p)) {
        displayError("Paquet rejete");
        return;
    }

    recordSequence(p.device_id, frame.sequence);

    Serial.println("------------------------------------------------");
    Serial.printf("Device ID  : 0x%08X\n", p.device_id);
    Serial.printf("Version    : %u\n", frame.version);
    Serial.printf("Seq        : %u\n", frame.sequence);
    Serial.printf("Flags      : 0x%02X\n", frame.flags);
    Serial.printf("Temperature: %.2f C\n", p.temperature);
    Serial.printf("Pression   : %.0f hPa\n", p.pressure);
    Serial.printf("Poussiere  : %.1f ug/m3\n", p.pm25);
    Serial.printf("RSSI       : %.1f dBm\n", frame.rssi);
    Serial.printf("SNR        : %.1f dB\n", frame.snr);
    Serial.println("------------------------------------------------");

    displayPacket(p.device_id, p.temperature, p.pressure, p.pm25, frame.rssi, frame.snr);

    wifiConnect();
    if (WiFi.status() == WL_CONNECTED) {
        if (!scSend(p))
            displayError("SC: envoi echoue");
    }
}
