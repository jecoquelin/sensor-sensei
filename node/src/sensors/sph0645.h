//
// Created by Moolinex on 11/06/2026.
//

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
│ SEL        │ GND  ← canal LEFT  │
└────────────┴────────────────────┘

Note : SEL à GND → données sur front montant de LRCL (canal gauche).
       SEL à 3.3V → canal droit.
*/

#define MIC_BCLK    32
#define MIC_WS      25      // LRCL
#define MIC_DATA    34      // DOUT

#define MIC_SAMPLE_RATE     16000                       // Hz
#define MIC_BITS_PER_SAMPLE 32                          // SPH0645 envoie 24 bits dans un mot de 32 bits
#define MIC_CHANNEL_FORMAT  I2S_CHANNEL_FMT_ONLY_LEFT  // SEL = GND
#define MIC_BUFFER_SIZE     1024                        // samples par lecture

bool   micInit();
size_t micRead(int32_t *buf, size_t samples);
float  micReadRMS(size_t samples);