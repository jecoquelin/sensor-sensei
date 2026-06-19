#include "dust.h"

// ─── Constructeur ────────────────────────────────────────────────────────────
DustSensor::DustSensor(uint8_t iledPin, uint8_t voutPin)
    : _iledPin(iledPin),
      _voutPin(voutPin),
      _lastVoltage(0.0f),
      _lastDensity(0.0f),
      _sum(0),
      _initialized(false)
{
    memset(_buff, 0, sizeof(_buff));
}

// ─── begin ───────────────────────────────────────────────────────────────────
void DustSensor::begin() {
    pinMode(_iledPin, OUTPUT);
    digitalWrite(_iledPin, LOW);

    // Chauffe de la LED IR — ignore les premières mesures
    Serial.println("[DUST] Warming up...");
    for (int i = 0; i < 10; i++) {
        digitalWrite(_iledPin, HIGH);
        delayMicroseconds(280);
        analogRead(_voutPin); // lecture jetée
        digitalWrite(_iledPin, LOW);
        delay(500);
    }
}

// ─── resetBuffer ─────────────────────────────────────────────────────────────
void DustSensor::resetBuffer() {
    _initialized = false;
    _sum = 0;
    memset(_buff, 0, sizeof(_buff));
}

// ─── read ─────────────────────────────────────────────────────────────────────
float DustSensor::read() {
    // 1. Allume la LED IR, attend le pic de réponse du capteur (280µs),
    //    lit l'ADC, puis éteint la LED.
    digitalWrite(_iledPin, HIGH);
    delayMicroseconds(280);
    int raw = analogRead(_voutPin);
    Serial.printf("[DUST DEBUG] raw ADC: %d\n", raw);
    digitalWrite(_iledPin, LOW);

    // 2. Applique le filtre par moyenne glissante
    int filtered = _filter(raw);

    // 3. Conversion ADC → tension (mV)
    // ESP32 : 12 bits (0–4095), 3.3V
    _lastVoltage = (DUST_SYS_VOLTAGE / DUST_ADC_RESOLUTION) * filtered;

    // 4. Exception : voltage à 0 → capteur probablement non alimenté ou pin flottante
    if (_lastVoltage == 0.0f) {
        Serial.println("[DUST] ERROR: voltage = 0 mV, capteur non alimenté ou VOUT flottant");
        _lastDensity = -1.0f; // valeur sentinelle pour signaler l'erreur en amont
        resetBuffer();
        return _lastDensity;
    }

    // 5. Tension → densité (ug/m³)
    // Si le voltage dépasse le seuil "air propre", on calcule la densité.
    if (_lastVoltage >= DUST_NO_DUST_VOLTAGE) {
        _lastDensity = (_lastVoltage - DUST_NO_DUST_VOLTAGE) * DUST_COV_RATIO;
    } else {
        _lastDensity = 0.0f;
    }

    return _lastDensity;
}

// ─── Getters ─────────────────────────────────────────────────────────────────
float DustSensor::getLastVoltage() const {
    return _lastVoltage;
}

float DustSensor::getLastDensity() const {
    return _lastDensity;
}

// ─── _filter (moyenne glissante sur DUST_FILTER_SIZE échantillons) ────────────
int DustSensor::_filter(int rawValue) {
    if (!_initialized) {
        // Premier appel : remplit le buffer avec la valeur initiale
        _initialized = true;
        _sum = 0;
        for (int i = 0; i < DUST_FILTER_SIZE; i++) {
            _buff[i] = rawValue;
            _sum += rawValue;
        }
        return rawValue;
    }

    // Retire la valeur la plus ancienne, insère la nouvelle
    _sum -= _buff[0];
    for (int i = 0; i < DUST_FILTER_SIZE - 1; i++) {
        _buff[i] = _buff[i + 1];
    }
    _buff[DUST_FILTER_SIZE - 1] = rawValue;
    _sum += rawValue;

    return _sum / DUST_FILTER_SIZE;
}