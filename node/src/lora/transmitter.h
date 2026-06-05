#pragma once
#include <stdint.h>
#include <stddef.h>

/*
Branchement SX1276 (intégré T-Beam)
┌────────┬────────┐
│ Signal │  GPIO  │
├────────┼────────┤
│ SCK    │ 5      │
│ MISO   │ 19     │
│ MOSI   │ 27     │
│ NSS    │ 18     │
│ RESET  │ 23     │
│ DIO0   │ 26     │
│ DIO1   │ 33     │
└────────┴────────┘
*/

#define LORA_NSS        18
#define LORA_RST        23
#define LORA_DIO0       26
#define LORA_DIO1       33

#define LORA_FREQ       868.0
#define LORA_BW         125.0
#define LORA_SF         9
#define LORA_CR         7
#define LORA_SYNC_WORD  0x12
#define LORA_POWER      14

bool loraInit();
bool loraSend(const uint8_t *buf, size_t len);

