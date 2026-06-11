#pragma once
#include <stdint.h>

/*
Branchement SPH0645 (I2S microphone)
┌─────────┬──────────────────────────────────────────┐
│ SPH0645 │                 T-Beam                   │
├─────────┼──────────────────────────────────────────┤
│ 3V      │ 3.3V                                     │
├─────────┼──────────────────────────────────────────┤
│ GND     │ GND                                      │
├─────────┼──────────────────────────────────────────┤
│ SEL     │ GND (canal gauche)                       │
├─────────┼──────────────────────────────────────────┤
│ BCLK    │ GPIO 32 (Bit Clock)                      │
├─────────┼──────────────────────────────────────────┤
│ LRCL    │ GPIO 25 (Word Select)                    │
├─────────┼──────────────────────────────────────────┤
│ DOUT    │ GPIO 35 (Data in — input only)           │
└─────────┴──────────────────────────────────────────┘

GPIO 34 réservé au GPS (UART RX) → utiliser GPIO 35
SEL = GND  → canal gauche
SEL = 3.3V → canal droit

Données 18 bits, left-justified dans un mot I2S 24 bits.
Sensibilité : -26 dBFS @ 94 dB SPL → offset calibration = +120 dB
*/

#define MIC_BCLK        32
#define MIC_LRCL        25
#define MIC_DOUT        35

#define MIC_SAMPLE_RATE 16000
#define MIC_SAMPLES     512

bool  micInit();
float micReadDb();  // retourne le niveau sonore approx. en dB SPL
