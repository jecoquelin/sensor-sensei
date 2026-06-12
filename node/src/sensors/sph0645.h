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
│ SEL        │ GND                │
└────────────┴────────────────────┘
*/

#define MIC_BCLK        32
#define MIC_WS          25
#define MIC_DATA        34

#define MIC_SAMPLE_RATE 32000
#define MIC_BUFFER_SIZE 1024

bool  micInit();
float micReadRMS();