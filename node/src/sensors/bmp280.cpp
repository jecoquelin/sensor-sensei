
#include "bmp280.h"
#include <Wire.h>
#include <Arduino.h>

static Adafruit_BMP280 _bmp;

bool bmp280Init() {
    Wire.begin(BMP280_SDA, BMP280_SCL);
    if (!_bmp.begin(BMP280_ADDR)) {
        Serial.println("[BMP280] Not found !");
        return false;
    }
    Serial.println("[BMP280] OK");
    return true;
}

float bmp280ReadTemperature() {
    return _bmp.readTemperature();
}

float bmp280ReadPressure() {
    return _bmp.readPressure() / 100.0f;  // Pa → hPa
}
