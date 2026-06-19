#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "display/display.h"
#include "lora/receiver.h"
#include "gatekeeper.h"
#include "http/sensor_community.h"

static void wifiConnect() {
    if (WiFi.status() == WL_CONNECTED) return;

    Serial.printf("[WiFi] Connexion à %s...\n", WIFI_SSID);
    displayStatus("Connexion WiFi...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 15000)
        delay(500);

    if (WiFi.status() == WL_CONNECTED)
        Serial.printf("[WiFi] Connecté — IP: %s\n", WiFi.localIP().toString().c_str());
    else
        Serial.println("[WiFi] Timeout — données non envoyées ce cycle");
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
    if (!loraReceive(frame)) return;

    const SensorPayload &p = frame.payload;

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

    wifiConnect();
    if (WiFi.status() == WL_CONNECTED) {
        if (!scSend(p))
            displayError("SC: envoi échoué");
    }
}
