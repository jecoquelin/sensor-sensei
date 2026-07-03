#pragma once

#include <stdint.h>
#include <stddef.h>
#include <driver/i2s.h>

/*
Branchement SPH0645 (micro I2S)
┌────────────┬────────────────────┐
│  SPH0645   │       T-Beam       │
├────────────┼────────────────────┤
│ 3V         │ 3.3V               │
├────────────┼────────────────────┤
│ GND        │ GND                │
├────────────┼────────────────────┤
│ BCLK       │ GPIO 32            │
├────────────┼────────────────────┤
│ LRCL (WS)  │ GPIO 25            │
├────────────┼────────────────────┤
│ DOUT       │ GPIO 34            │
├────────────┼────────────────────┤
│ SEL        │ GND                │
└────────────┴────────────────────┘
*/

// ─── Pins ────────────────────────────────────────────────────────────────────
#define MIC_BCLK        32
#define MIC_WS          25
#define MIC_DATA        34

// ─── Acquisition ──────────────────────────────────────────────────────────────
#define MIC_SAMPLE_RATE  32000
#define MIC_BUFFER_SIZE  1024
#define MIC_I2S_PORT     I2S_NUM_0

// ─── Classe ──────────────────────────────────────────────────────────────────
class MicrophoneSensor {
public:
    MicrophoneSensor(uint8_t bclkPin = MIC_BCLK, uint8_t wsPin = MIC_WS, uint8_t dataPin = MIC_DATA);

    // Installe le driver I2S en mode maître/RX et configure les pins.
    // À appeler une seule fois dans setup().
    bool begin();

    // Lit un bloc d'échantillons I2S et retourne le niveau RMS (0.0–1.0).
    // Bloquant le temps d'un buffer (~32 ms à 32 kHz / 1024 échantillons).
    float read();

    // Niveau RMS calculé lors du dernier appel à read() — sans refaire de mesure
    float getLastRMS() const;

private:
    uint8_t _bclkPin;
    uint8_t _wsPin;
    uint8_t _dataPin;

    float _lastRMS;
};
