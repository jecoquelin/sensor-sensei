#include "dust.h"
#include <cstring>

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

    // Le capteur GP2Y1010 a besoin de quelques impulsions LED pour stabiliser
    // son amplificateur interne avant que les mesures soient fiables.
    Serial.println("[DUST] Warming up...");
    for (int i = 0; i < 5; i++) {
        digitalWrite(_iledPin, HIGH);
        delayMicroseconds(280);     // durée d'impulsion recommandée par la datasheet
        analogRead(_voutPin);       // lecture jetée pendant la chauffe
        digitalWrite(_iledPin, LOW);
        delay(300);
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
    // 1. Impulsion LED IR de 280 µs — délai critique pour la réponse optique du capteur
    digitalWrite(_iledPin, HIGH);
    delayMicroseconds(280);
    int raw = analogRead(_voutPin);
    Serial.printf("[DUST DEBUG] raw ADC: %d\n", raw);
    digitalWrite(_iledPin, LOW);

    // 2. Lissage par moyenne glissante pour réduire le bruit ADC
    int filtered = _filter(raw);

    // 3. Conversion ADC → tension en mV
    // ESP32 ADC 12 bits (0–4095) sur 3.3V — mais le pont diviseur 10k/20k
    // ramène le VOUT 5V du capteur dans cette plage.
    _lastVoltage = (DUST_SYS_VOLTAGE / DUST_ADC_RESOLUTION) * filtered;

    // 4. Tension nulle = capteur non alimenté ou pin flottante
    if (_lastVoltage == 0.0f) {
        Serial.println("[DUST] ERROR: voltage = 0 mV, capteur non alimenté ou VOUT flottant");
        _lastDensity = -1.0f;  // valeur sentinelle pour signaler l'erreur en amont
        resetBuffer();
        return _lastDensity;
    }

    // 5. Tension → densité µg/m³
    // En dessous du seuil NO_DUST_VOLTAGE, l'air est considéré propre (densité = 0).
    // Au-dessus, la relation tension/densité est linéaire selon le ratio du fabricant.
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

// ─── _filter : moyenne glissante sur DUST_FILTER_SIZE échantillons ────────────
int DustSensor::_filter(int rawValue) {
    if (!_initialized) {
        // Premier appel : remplit le buffer avec la valeur initiale pour éviter
        // une montée progressive artificielle au démarrage.
        _initialized = true;
        _sum = 0;
        for (int i = 0; i < DUST_FILTER_SIZE; i++) {
            _buff[i] = rawValue;
            _sum += rawValue;
        }
        return rawValue;
    }

    // Fenêtre glissante FIFO : retire l'échantillon le plus ancien, insère le nouveau
    _sum -= _buff[0];
    for (int i = 0; i < DUST_FILTER_SIZE - 1; i++) {
        _buff[i] = _buff[i + 1];
    }
    _buff[DUST_FILTER_SIZE - 1] = rawValue;
    _sum += rawValue;

    return _sum / DUST_FILTER_SIZE;
}
