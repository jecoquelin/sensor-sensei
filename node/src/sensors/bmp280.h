#pragma once
#include <Adafruit_BMP280.h>

/*
Branchement BMP280 (HW-611)
┌────────┬────────────────────┐
│ HW-611 │       T-Beam       │
├────────┼────────────────────┤
│ VCC    │ 3.3V               │
├────────┼────────────────────┤
│ GND    │ GND                │
├────────┼────────────────────┤
│ SDA    │ GPIO 21            │
├────────┼────────────────────┤
│ SCL    │ GPIO 22            │
├────────┼────────────────────┤
│ CSB    │ 3.3V ← critique    │
├────────┼────────────────────┤
│ SDD    │ GND (adresse 0x76) │
└────────┴────────────────────┘
*/

#define BMP280_SDA  21
#define BMP280_SCL  22
#define BMP280_ADDR 0x76

bool    bmp280Init();
float   bmp280ReadTemperature();
float   bmp280ReadPressure();

