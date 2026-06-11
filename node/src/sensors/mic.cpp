#include "mic.h"
#include <driver/i2s.h>
#include <Arduino.h>
#include <math.h>

#define I2S_PORT I2S_NUM_0

bool micInit() {
    i2s_config_t cfg = {
        .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate          = MIC_SAMPLE_RATE,
        .bits_per_sample      = I2S_BITS_PER_SAMPLE_32BIT,
        .channel_format       = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count        = 4,
        .dma_buf_len          = MIC_SAMPLES,
        .use_apll             = false,
        .tx_desc_auto_clear   = false,
        .fixed_mclk           = 0,
    };

    i2s_pin_config_t pins = {
        .bck_io_num   = MIC_BCLK,
        .ws_io_num    = MIC_LRCL,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num  = MIC_DOUT,
    };

    if (i2s_driver_install(I2S_PORT, &cfg, 0, NULL) != ESP_OK) {
        Serial.println("[MIC] Driver install failed");
        return false;
    }
    if (i2s_set_pin(I2S_PORT, &pins) != ESP_OK) {
        Serial.println("[MIC] Pin config failed");
        return false;
    }

    // Le SPH0645 a besoin de quelques cycles WS avant de débiter
    delay(100);
    int32_t discard[MIC_SAMPLES];
    size_t  dummy;
    i2s_read(I2S_PORT, discard, sizeof(discard), &dummy, portMAX_DELAY);

    Serial.println("[MIC] OK");
    return true;
}

float micReadDb() {
    int32_t buf[MIC_SAMPLES];
    size_t  bytes_read = 0;

    i2s_read(I2S_PORT, buf, sizeof(buf), &bytes_read, portMAX_DELAY);
    int samples = bytes_read / sizeof(int32_t);

    // Diagnostic : affiche les 3 premières valeurs brutes
    // Si tout est 0x00000000 → problème de câblage
    // Si valeurs non nulles → problème de shift uniquement
    Serial.printf("[MIC] raw: 0x%08X  0x%08X  0x%08X\n",
                  (uint32_t)buf[0], (uint32_t)buf[1], (uint32_t)buf[2]);

    // SPH0645 : 18 bits utiles left-justified → shift >>14
    double sum = 0.0;
    for (int i = 0; i < samples; i++) {
        int32_t s = buf[i] >> 14;
        sum += (double)s * s;
    }

    double rms = sqrt(sum / samples);
    if (rms < 1.0) rms = 1.0;

    return 20.0f * log10f((float)rms / 131072.0f) + 120.0f;
}
