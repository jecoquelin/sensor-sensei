#include "display.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

static Adafruit_SSD1306 _oled(SCREEN_W, SCREEN_H, &Wire, OLED_RST);

void displayInit() {
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

void displayPacket(uint16_t device_id, float temp, float pressure, float dust, float rssi, float snr) {
    _oled.clearDisplay();
    _oled.setTextSize(1);
    _oled.setCursor(0, 0);

    // Line 0: title
    _oled.println("=== LoRa Gateway ===");

    // Line 1: device id
    char buf[22];
    snprintf(buf, sizeof(buf), "Dev : 0x%04X", device_id);
    _oled.println(buf);

    // Line 2: temperature
    snprintf(buf, sizeof(buf), "Temp: %.1f C", temp);
    _oled.println(buf);

    // Line 3: pressure
    snprintf(buf, sizeof(buf), "Pres: %.0f hPa", pressure);
    _oled.println(buf);

    // Line 4: pressure
    snprintf(buf, sizeof(buf), "Dust: %.1f µ/m³", dust);
    _oled.println(buf);

    // Line 5: RSSI
    snprintf(buf, sizeof(buf), "RSSI: %.1f dBm", rssi);
    _oled.println(buf);

    // Line 6: SNR
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
