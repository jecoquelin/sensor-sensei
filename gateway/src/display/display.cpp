#include "display.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

static Adafruit_SSD1306 _oled(SCREEN_W, SCREEN_H, &Wire, OLED_RST);

void displayInit() {
    // Le SSD1306 du Heltec V3 nécessite une impulsion RST manuelle au démarrage.
    // Sans ce reset, l'écran reste noir même si I2C répond.
    pinMode(OLED_RST, OUTPUT);
    digitalWrite(OLED_RST, LOW);
    delay(10);
    digitalWrite(OLED_RST, HIGH);

    Wire.begin(OLED_SDA, OLED_SCL);
    if (!_oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
        Serial.println("[OLED] Init failed");
        return;
    }
    _oled.clearDisplay();
    _oled.setTextColor(SSD1306_WHITE);
    _oled.setTextSize(1);
    _oled.setCursor(0, 0);
    _oled.println("Gateway LoRa 868MHz");
    _oled.println("En attente...");
    _oled.display();
}

void displayStatus(const char *status) {
    _oled.clearDisplay();
    _oled.setTextSize(1);
    _oled.setCursor(0, 0);
    _oled.println("Gateway LoRa 868MHz");
    _oled.println(status);
    _oled.display();
}

void displayPacket(uint32_t device_id, float temp, float pressure, float dust, float mic, float rssi, float snr) {
    _oled.clearDisplay();
    _oled.setTextSize(1);
    _oled.setCursor(0, 0);

    // L'écran SSD1306 128×64 avec textSize(1) affiche 8 lignes de 21 caractères max
    char buf[22];

    _oled.println("=== LoRa Gateway ===");

    snprintf(buf, sizeof(buf), "Dev:%08X", device_id);
    _oled.println(buf);

    snprintf(buf, sizeof(buf), "Temp: %.1f C", temp);
    _oled.println(buf);

    snprintf(buf, sizeof(buf), "Pres: %.0f hPa", pressure);
    _oled.println(buf);

    snprintf(buf, sizeof(buf), "Dust: %.1f µ/m³", dust);
    _oled.println(buf);

    snprintf(buf, sizeof(buf), "Mic : %.4f", mic);
    _oled.println(buf);

    snprintf(buf, sizeof(buf), "RSSI: %.1f dBm", rssi);
    _oled.println(buf);

    snprintf(buf, sizeof(buf), "SNR : %.1f dB", snr);
    _oled.println(buf);

    _oled.display();
}

void displayError(const char *msg) {
    _oled.clearDisplay();
    _oled.setTextSize(1);
    _oled.setCursor(0, 0);
    _oled.println("=== LoRa Gateway ===");
    _oled.println("[ERREUR]");
    _oled.println(msg);
    _oled.display();
}
