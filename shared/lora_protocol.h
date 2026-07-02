#pragma once

#include <stddef.h>
#include <stdint.h>

#include "payload.h"

static constexpr uint8_t LORA_PROTOCOL_VERSION = 1;
static constexpr uint8_t LORA_PROTOCOL_FLAG_NONE = 0x00;
static constexpr size_t  LORA_PROTOCOL_HEADER_LEN = 4;
static constexpr size_t  LORA_PROTOCOL_CRC_LEN = 2;
static constexpr size_t  LORA_PROTOCOL_FRAME_LEN =
    LORA_PROTOCOL_HEADER_LEN + PAYLOAD_LEN + LORA_PROTOCOL_CRC_LEN;

enum LoraProtocolError : uint8_t {
    LORA_PROTOCOL_OK = 0,
    LORA_PROTOCOL_ERR_LENGTH,
    LORA_PROTOCOL_ERR_VERSION,
    LORA_PROTOCOL_ERR_PAYLOAD_LENGTH,
    LORA_PROTOCOL_ERR_CRC,
};

inline uint16_t loraProtocolCrc16Ccitt(const uint8_t *data, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; ++i) {
        crc ^= (uint16_t)data[i] << 8;
        for (uint8_t bit = 0; bit < 8; ++bit) {
            if (crc & 0x8000)
                crc = (crc << 1) ^ 0x1021;
            else
                crc <<= 1;
        }
    }
    return crc;
}

inline size_t loraProtocolEncodeFrame(uint8_t *buf, uint8_t sequence, uint8_t flags, const uint8_t *payload, size_t payload_len) {
    buf[0] = LORA_PROTOCOL_VERSION;
    buf[1] = sequence;
    buf[2] = flags;
    buf[3] = (uint8_t)payload_len;

    for (size_t i = 0; i < payload_len; ++i)
        buf[LORA_PROTOCOL_HEADER_LEN + i] = payload[i];

    uint16_t crc = loraProtocolCrc16Ccitt(buf, LORA_PROTOCOL_HEADER_LEN + payload_len);
    size_t crcIndex = LORA_PROTOCOL_HEADER_LEN + payload_len;
    buf[crcIndex] = (crc >> 8) & 0xFF;
    buf[crcIndex + 1] = crc & 0xFF;
    return crcIndex + 2;
}

inline LoraProtocolError loraProtocolDecodeFrame(const uint8_t *buf, size_t len, uint8_t &sequence, uint8_t &flags, SensorPayload &payload) {
    if (len != LORA_PROTOCOL_FRAME_LEN)
        return LORA_PROTOCOL_ERR_LENGTH;

    if (buf[0] != LORA_PROTOCOL_VERSION)
        return LORA_PROTOCOL_ERR_VERSION;

    if (buf[3] != PAYLOAD_LEN)
        return LORA_PROTOCOL_ERR_PAYLOAD_LENGTH;

    uint16_t expectedCrc = loraProtocolCrc16Ccitt(buf, LORA_PROTOCOL_HEADER_LEN + PAYLOAD_LEN);
    uint16_t receivedCrc = ((uint16_t)buf[LORA_PROTOCOL_HEADER_LEN + PAYLOAD_LEN] << 8)
                         | (uint16_t)buf[LORA_PROTOCOL_HEADER_LEN + PAYLOAD_LEN + 1];
    if (expectedCrc != receivedCrc)
        return LORA_PROTOCOL_ERR_CRC;

    sequence = buf[1];
    flags = buf[2];
    decodePayload(buf + LORA_PROTOCOL_HEADER_LEN, payload);
    return LORA_PROTOCOL_OK;
}

inline const char *loraProtocolErrorString(LoraProtocolError error) {
    switch (error) {
        case LORA_PROTOCOL_OK: return "OK";
        case LORA_PROTOCOL_ERR_LENGTH: return "bad length";
        case LORA_PROTOCOL_ERR_VERSION: return "bad version";
        case LORA_PROTOCOL_ERR_PAYLOAD_LENGTH: return "bad payload length";
        case LORA_PROTOCOL_ERR_CRC: return "crc mismatch";
        default: return "unknown";
    }
}
