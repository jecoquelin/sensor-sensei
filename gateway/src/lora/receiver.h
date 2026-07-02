#pragma once

#include <stdint.h>

#include "../../shared/lora_protocol.h"

/*
Branchement SX1262 (intégré Heltec V3 - interne)
┌────────┬─────────┐
│ Signal │  GPIO   │
├────────┼─────────┤
│ SCK    │ 9       │
│ MISO   │ 11      │
│ MOSI   │ 10      │
│ NSS    │ 8       │
│ RESET  │ 12      │
│ DIO1   │ 14      │
│ BUSY   │ 13      │
└────────┴─────────┘
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
    uint8_t       version;
    uint8_t       sequence;
    uint8_t       flags;
    SensorPayload payload;
    float         rssi;
    float         snr;
};

bool loraInit();
bool loraReceive(LoRaFrame &frame);
