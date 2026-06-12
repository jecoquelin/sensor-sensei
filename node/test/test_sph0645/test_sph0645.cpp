//
// Created by Moolinex on 11/06/2026.
//

#include "sensors/sph0645.h"
#include <driver/i2s.h>
#include <Arduino.h>
#include <math.h>

#define I2S_PORT    I2S_NUM_0

static int32_t _mic_buf[MIC_BUFFER_SIZE];

bool micInit() {
    Serial.println("[MIC] Step 1 — i2s_driver_install...");
    i2s_config_t cfg = {
        .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate          = 48000,
        .bits_per_sample      = I2S_BITS_PER_SAMPLE_32BIT,
        .channel_format       = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count        = 8,
        .dma_buf_len          = 256,
        .use_apll             = true,
        .tx_desc_auto_clear   = false,
        .fixed_mclk           = 0
    };
    if (i2s_driver_install(I2S_PORT, &cfg, 0, NULL) != ESP_OK) {
        Serial.println("[MIC] Step 1 FAIL");
        return false;
    }
    Serial.println("[MIC] Step 1 PASS");

    Serial.println("[MIC] Step 2 — i2s_set_pin...");
    i2s_pin_config_t pins = {
        .bck_io_num   = MIC_BCLK,
        .ws_io_num    = MIC_WS,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num  = MIC_DATA
    };
    if (i2s_set_pin(I2S_PORT, &pins) != ESP_OK) {
        Serial.println("[MIC] Step 2 FAIL");
        i2s_driver_uninstall(I2S_PORT);
        return false;
    }
    Serial.println("[MIC] Step 2 PASS");

    Serial.println("[MIC] Step 3 — flush buffer démarrage...");
    size_t dummy;
    i2s_read(I2S_PORT, _mic_buf, sizeof(_mic_buf), &dummy, 200 / portTICK_PERIOD_MS);
    Serial.printf("[MIC] Step 3 PASS — %d bytes flushed\n", dummy);

    Serial.println("[MIC] OK");
    return true;
}

size_t micRead(int32_t *buf, size_t samples) {
    size_t bytes_read = 0;
    i2s_read(I2S_PORT, buf, samples * sizeof(int32_t), &bytes_read, portMAX_DELAY);
    return bytes_read / sizeof(int32_t);
}

float micReadRMS(size_t samples) {
    if (samples > MIC_BUFFER_SIZE) samples = MIC_BUFFER_SIZE;

    size_t n = micRead(_mic_buf, samples);

    // Step 4 — nombre de samples
    Serial.printf("[MIC] Step 4 samples lus : %d — %s\n", n, n > 0 ? "PASS" : "FAIL");

    // Step 5 — contenu du buffer
    bool all_minus_one = true;
    bool all_zero = true;
    for (size_t i = 0; i < n; i++) {
        if (_mic_buf[i] != -1) all_minus_one = false;
        if (_mic_buf[i] != 0)  all_zero = false;
    }
    Serial.printf("[MIC] Step 5 buffer : %s\n",
        all_minus_one ? "FAIL — que des -1 (DATA flottant ou micro HS)" :
        all_zero      ? "FAIL — que des 0 (sleep mode, BCLK trop lent)" :
                        "PASS — données valides");

    // Step 6 — samples bruts
    Serial.printf("[MIC] Step 6 raw[0..7] : ");
    for (int i = 0; i < 8 && i < (int)n; i++) {
        Serial.printf("%ld ", _mic_buf[i]);
    }
    Serial.println();

    if (n == 0) return 0.0f;

    // Step 7 — calcul RMS
    double sum = 0.0;
    for (size_t i = 0; i < n; i++) {
        float sample = (float)(_mic_buf[i] >> 14) / (float)(1 << 17);
        sum += sample * sample;
        _mic_buf[i] = 0;
    }
    float rms = sqrtf((float)(sum / n));
    Serial.printf("[MIC] Step 7 RMS : %.6f — %s\n",
        rms, rms > 0.001f ? "PASS" : "FAIL — signal trop faible");
    Serial.println("─────────────────────────────");

    return rms;
}