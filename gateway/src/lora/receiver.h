
#pragma once
#include <stdint.h>
#include "../../shared/payload.h"

/*
Branchement SX1262 (intégré Heltec V3 - interne)
┌────────┬────────┐
│ Signal │  GPIO  │
├────────┼────────┤
│ SCK    │ 9      │
│ MISO   │ 11     │
│ MOSI   │ 10     │
│ NSS    │ 8      │
│ RESET  │ 12     │
│ DIO1   │ 14     │
│ BUSY   │ 13     │
└────────┴────────┘

RSSI - Plus c'est proche de 0, meilleur c'est
SNR - rapport signal/bruit, en dB
┌────────────┬────────────┬──────────┬────────────┐
│ Indicateur │    Bon     │  Limite  │    Mort    │
├────────────┼────────────┼──────────┼────────────┤
│ RSSI       │ > -100 dBm │ -110 dBm │ < -120 dBm │
├────────────┼────────────┼──────────┼────────────┤
│ SNR        │ > 0 dB     │ -10 dB   │ < -20 dB   │
└────────────┴────────────┴──────────┴────────────┘
*/

#define LORA_NSS        8
#define LORA_DIO1       14
#define LORA_RST        12
#define LORA_BUSY       13

#define LORA_FREQ       868.0
#define LORA_BW         125.0
#define LORA_SF         9
#define LORA_CR         7
#define LORA_SYNC_WORD  0x12

struct LoRaFrame {
    SensorPayload payload;
    float         rssi;
    float         snr;
};

bool loraInit();
bool loraReceive(LoRaFrame &frame);  // retourne true si une trame est dispo
