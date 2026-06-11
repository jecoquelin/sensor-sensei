#pragma once
#include <stdint.h>

// ─── OLED SSD1306 (Heltec WiFi LoRa 32 V3) ───────────────────────────────────
// SDA : GPIO 17
// SCL : GPIO 18
// RST : GPIO 21
#define OLED_SDA 17
#define OLED_SCL 18
#define OLED_RST 21
#define OLED_ADDR 0x3C
#define SCREEN_W 128
#define SCREEN_H 64

void displayInit();
void displayStatus(const char *status);
void displayPacket(uint16_t device_id, float temp, float pressure, float noise_db, float rssi, float snr);
void displayError(const char *msg);
