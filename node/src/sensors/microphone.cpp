#include "microphone.h"
#include <Arduino.h>
#include <math.h>

static int32_t _raw[MIC_BUFFER_SIZE * 2];

// ─── Constructeur ────────────────────────────────────────────────────────────
MicrophoneSensor::MicrophoneSensor(uint8_t bclkPin, uint8_t wsPin, uint8_t dataPin)
    : _bclkPin(bclkPin),
      _wsPin(wsPin),
      _dataPin(dataPin),
      _lastRMS(0.0f)
{
}

// ─── begin ───────────────────────────────────────────────────────────────────
bool MicrophoneSensor::begin() {
    // Au cas où un driver I2S serait déjà installé sur ce port (ex: réveil depuis
    // deep sleep sans reset propre).
    i2s_driver_uninstall(MIC_I2S_PORT);

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
        .bck_io_num   = _bclkPin,
        .ws_io_num    = _wsPin,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num  = _dataPin
    };

    if (i2s_driver_install(MIC_I2S_PORT, &i2s_config, 0, NULL) != ESP_OK) {
        Serial.println("[MIC] driver install failed");
        return false;
    }
    if (i2s_set_pin(MIC_I2S_PORT, &pin_config) != ESP_OK) {
        Serial.println("[MIC] set pin failed");
        return false;
    }

    i2s_zero_dma_buffer(MIC_I2S_PORT);
    Serial.println("[MIC] OK");
    return true;
}

// ─── read ────────────────────────────────────────────────────────────────────
float MicrophoneSensor::read() {
    size_t bytes_read = 0;
    i2s_read(MIC_I2S_PORT, _raw, sizeof(_raw), &bytes_read, portMAX_DELAY);
    size_t n = bytes_read / sizeof(int32_t);

    if (n == 0) {
        _lastRMS = 0.0f;
        return _lastRMS;
    }

    // Le SPH0645 renvoie des échantillons 18 bits alignés en MSB sur 32 bits —
    // on décale de 14 bits pour ramener la valeur utile en tête, puis on
    // normalise sur l'échelle 18 bits (1 << 17) pour obtenir une amplitude [-1, 1].
    double sum = 0.0;
    size_t count = 0;
    for (size_t i = 0; i < n; i++) {
        if (_raw[i] == -1) continue;  // échantillon invalide (DATA flottant)
        float s = (float)(_raw[i] >> 14) / (float)(1 << 17);
        sum += s * s;
        count++;
    }

    if (count == 0) {
        _lastRMS = 0.0f;
        return _lastRMS;
    }

    _lastRMS = sqrtf((float)(sum / count));
    Serial.printf("[MIC] RMS: %.4f | %.1f dBFS\n",
                  _lastRMS, 20.0f * log10f(_lastRMS > 0.0f ? _lastRMS : 1e-9f));
    return _lastRMS;
}

// ─── Getters ─────────────────────────────────────────────────────────────────
float MicrophoneSensor::getLastRMS() const {
    return _lastRMS;
}
