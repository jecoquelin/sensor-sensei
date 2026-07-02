/*
 * SensorSensei - Gateway (Heltec WiFi LoRa 32 V3)
 *
 * Le gateway ecoute en permanence sur 868 MHz. A chaque paquet recu :
 *   1. Verifie que l'expediteur est autorise (gatekeeper)
 *   2. Affiche les donnees sur l'ecran OLED
 *   3. Met les mesures en file si le POST HTTP ne peut pas partir tout de suite
 *
 * Contrairement au node, le gateway est alimente sur USB et tourne en continu.
 */

#include <Arduino.h>
#include <WiFi.h>

#include "config.h"
#include "display/display.h"
#include "gatekeeper.h"
#include "http/outbox.h"
#include "lora/receiver.h"

static void wifiConnect(uint32_t timeoutMs = 15000) {
    if (WiFi.status() == WL_CONNECTED) return;

    WiFi.mode(WIFI_STA);
    WiFi.persistent(false);
    WiFi.setAutoReconnect(true);

    Serial.printf("[WiFi] Connexion a %s...\n", WIFI_SSID);
    displayStatus("Connexion WiFi...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < timeoutMs)
        delay(250);

    if (WiFi.status() == WL_CONNECTED)
        Serial.printf("[WiFi] Connecte - IP: %s\n", WiFi.localIP().toString().c_str());
    else
        Serial.println("[WiFi] Connexion initiale en echec, reprise en arriere-plan");
}

static void wifiMaintain() {
    static unsigned long lastReconnectAttempt = 0;

    if (WiFi.status() == WL_CONNECTED) return;
    if (millis() - lastReconnectAttempt < 30000) return;

    lastReconnectAttempt = millis();
    Serial.println("[WiFi] tentative de reconnexion en arriere-plan");
    WiFi.reconnect();
}

void setup() {
    Serial.begin(115200);

    displayInit();
    wifiConnect();
    httpOutboxInit();

    if (!loraInit()) {
        displayError("LoRa init failed");
        while (1);
    }

    displayStatus("Ecoute 868 MHz...");
}

void loop() {
    wifiMaintain();

    LoRaFrame frame;
    if (loraReceive(frame)) {
        const SensorPayload &p = frame.payload;

        if (!gatekeeperValidate(p)) {
            displayError("Paquet rejete");
        } else {
            Serial.println("------------------------------------------------");
            Serial.printf("Device ID  : 0x%08X\n", p.device_id);
            Serial.printf("Temperature: %.2f C\n", p.temperature);
            Serial.printf("Pression   : %.0f hPa\n", p.pressure);
            Serial.printf("Poussiere  : %.1f ug/m3\n", p.pm25);
            Serial.printf("RSSI       : %.1f dBm\n", frame.rssi);
            Serial.printf("SNR        : %.1f dB\n", frame.snr);
            Serial.println("------------------------------------------------");

            displayPacket(p.device_id, p.temperature, p.pressure, p.pm25, frame.rssi, frame.snr);
            httpOutboxEnqueueMeasurement(p);
            displayStatus("Mesure mise en file");
        }
    }

    httpOutboxTick();
}
