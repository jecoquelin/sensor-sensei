//
// Created by Moolinex on 11/06/2026.
//
#include "sph0645.h"
#include <driver/i2s.h>
#include <Arduino.h>
#include <math.h>

#define I2S_PORT    I2S_NUM_0

bool micInit() {
    i2s_config_t cfg = {
        .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate          = MIC_SAMPLE_RATE,
        .bits_per_sample      = I2S_BITS_PER_SAMPLE_32BIT,
        .channel_format       = MIC_CHANNEL_FORMAT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count        = 4,
        .dma_buf_len          = MIC_BUFFER_SIZE,
        .use_apll             = false,
        .tx_desc_auto_clear   = false,
        .fixed_mclk           = 0
    };

    i2s_pin_config_t pins = {
        .bck_io_num   = MIC_BCLK,
        .ws_io_num    = MIC_WS,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num  = MIC_DATA
    };

    if (i2s_driver_install(I2S_PORT, &cfg, 0, NULL) != ESP_OK) {
        Serial.println("[MIC] i2s_driver_install failed");
        return false;
    }
    if (i2s_set_pin(I2S_PORT, &pins) != ESP_OK) {
        Serial.println("[MIC] i2s_set_pin failed");
        i2s_driver_uninstall(I2S_PORT);
        return false;
    }

    REG_SET_BIT(I2S_TIMING_REG(I2S_PORT), BIT(9));
    REG_SET_BIT(I2S_CONF_REG(I2S_PORT),   I2S_RX_MSB_SHIFT);

    Serial.println("[MIC] OK");
    return true;
}

size_t micRead(int32_t *buf, size_t samples) {
    size_t bytes_read = 0;
    i2s_read(I2S_PORT, buf, samples * sizeof(int32_t), &bytes_read, portMAX_DELAY);
    return bytes_read / sizeof(int32_t);
}

float micReadRMS(size_t samples) {
    int32_t buf[samples];
    size_t  n = micRead(buf, samples);
    if (n == 0) return 0.0f;

    double sum = 0.0;
    for (size_t i = 0; i < n; i++) {
        float sample = (float)(buf[i] >> 8) / (float)(1 << 23);
        sum += sample * sample;
    }
    return sqrtf((float)(sum / n));
}

