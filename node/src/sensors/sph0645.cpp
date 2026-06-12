//
// Created by Moolinex on 11/06/2026.
//

#include "sensors/sph0645.h"
#include <driver/i2s.h>
#include <Arduino.h>
#include <math.h>

#define I2S_PORT I2S_NUM_0

static int32_t _raw[MIC_BUFFER_SIZE * 2];

bool micInit() {
    i2s_driver_uninstall(I2S_PORT);

    const i2s_config_t i2s_config = {
        .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate          = MIC_SAMPLE_RATE,
        .bits_per_sample      = I2S_BITS_PER_SAMPLE_32BIT,
        .channel_format       = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags     = 0,
        .dma_buf_count        = 16,
        .dma_buf_len          = 512,
        .use_apll             = false,
        .tx_desc_auto_clear   = false,
        .fixed_mclk           = 0
    };

    const i2s_pin_config_t pin_config = {
        .bck_io_num   = MIC_BCLK,
        .ws_io_num    = MIC_WS,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num  = MIC_DATA
    };

    if (i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL) != ESP_OK) {
        Serial.println("[MIC] driver install failed");
        return false;
    }
    if (i2s_set_pin(I2S_PORT, &pin_config) != ESP_OK) {
        Serial.println("[MIC] set pin failed");
        return false;
    }

    i2s_zero_dma_buffer(I2S_PORT);
    Serial.println("[MIC] OK");
    return true;
}

float micReadRMS() {
    size_t bytes_read = 0;
    i2s_read(I2S_PORT, _raw, sizeof(_raw), &bytes_read, portMAX_DELAY);
    size_t n = bytes_read / sizeof(int32_t);

    if (n == 0) return 0.0f;

    double sum = 0.0;
    size_t count = 0;
    for (size_t i = 0; i < n; i++) {
        if (_raw[i] == -1) continue;
        float s = (float)(_raw[i] >> 14) / (float)(1 << 17);
        sum += s * s;
        count++;
    }

    if (count == 0) return 0.0f;

    float result = sqrtf((float)(sum / count));
    Serial.printf("[MIC RAW] %.4f | %.1f dBFS\n", result, 20.0f * log10f(result > 0.0f ? result : 1e-9f));
    return result;
}