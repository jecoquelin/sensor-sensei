/*
 * SensorSensei — Gateway (Heltec WiFi LoRa 32 V3)
 *
 * Le gateway écoute en permanence sur 868 MHz. À chaque paquet reçu :
 *   1. Vérifie que l'expéditeur est autorisé (gatekeeper)
 *   2. Affiche les données sur l'écran OLED
 *   3. Envoie les données à l'API sensor.community via HTTPS
 *
 * Contrairement au node, le gateway est alimenté sur USB et tourne en continu.
 */

#include <Arduino.h>
#include <WiFi.h>
#include <cstring>
#include "config.h"
#include "display/display.h"
#include "lora/receiver.h"
#include "gatekeeper.h"
#include "http/sensor_community.h"
#include "portal/portal.h"

static void restartGateway(const char *reason) {
    Serial.printf("[SYS] %s\n", reason);
    displayError(reason);
    delay(2000);
    ESP.restart();
}

// Tente de (re)connecter le WiFi. Appelé au setup et avant chaque envoi HTTP
// car la connexion peut tomber entre deux paquets LoRa.
static void wifiConnect() {
    if (strlen(WIFI_SSID) == 0 || strlen(WIFI_PASSWORD) == 0) {
        Serial.println("[WiFi] No station credentials configured");
        portalSetStationStatus(false, "-", WiFi.localIP(), "No station credentials configured");
        return;
    }

    if (WiFi.status() == WL_CONNECTED) return;

    Serial.printf("[WiFi] Connexion à %s...\n", WIFI_SSID);
    displayStatus("Connexion WiFi...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    // Timeout 15 s — si le WiFi est indisponible, le gateway continue d'écouter
    // le LoRa et retente à la prochaine réception de paquet.
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 15000)
        delay(500);

    if (WiFi.status() == WL_CONNECTED)
        Serial.printf("[WiFi] Connecté — IP: %s\n", WiFi.localIP().toString().c_str());
    else
        Serial.println("[WiFi] Timeout — données non envoyées ce cycle");

    portalSetStationStatus(
        WiFi.status() == WL_CONNECTED,
        WIFI_SSID,
        WiFi.localIP(),
        WiFi.status() == WL_CONNECTED ? "Connected" : "Timeout"
    );
}

void setup() {
    Serial.begin(115200);

    displayInit();
    if (!portalInit()) {
        restartGateway("AP init failed");
    }
    wifiConnect();

    // LoRa doit être initialisé avant d'entrer dans loop()
    if (!loraInit()) {
        restartGateway("LoRa init failed");
    }
    portalSetLoraStatus(true, "Listening 868 MHz");
    displayStatus("Ecoute 868 MHz...");
}

void loop() {
    portalLoop();

    LoRaFrame frame;

    // loraReceive() est non-bloquant — retourne false si aucun paquet n'est disponible
    if (!loraReceive(frame)) return;

    const SensorPayload &p = frame.payload;

    // Valide l'expéditeur et les plages de valeurs capteur
    if (!gatekeeperValidate(p)) {
        displayError("Paquet rejeté");
        return;
    }

    Serial.println("─────────────────────────────");
    Serial.printf("Device ID  : 0x%08X\n", p.device_id);
    Serial.printf("Température: %.2f °C\n",  p.temperature);
    Serial.printf("Pression   : %.0f hPa\n", p.pressure);
    Serial.printf("Poussière  : %.1f µg/m³\n", p.pm25);
    Serial.printf("RSSI       : %.1f dBm\n", frame.rssi);
    Serial.printf("SNR        : %.1f dB\n",  frame.snr);
    Serial.println("─────────────────────────────");

    displayPacket(p.device_id, p.temperature, p.pressure, p.pm25, frame.rssi, frame.snr);
    portalSetLastPacket(p, frame.rssi, frame.snr);

    // Reconnexion WiFi si nécessaire avant l'envoi HTTP
    wifiConnect();
    if (WiFi.status() == WL_CONNECTED) {
        if (!scSend(p))
            displayError("SC: envoi échoué");
    }
}
