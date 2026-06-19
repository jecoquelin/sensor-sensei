#pragma once

#include <Arduino.h>

/*
Branchement Waveshare Dust Sensor
┌──────────────┬─────────────────────────────────────────────┐
│ Dust Sensor  │                   T-Beam                    │
├──────────────┼─────────────────────────────────────────────┤
│ VCC          │ 5V ← capteur conçu pour 5V, pas 3.3V        │
├──────────────┼─────────────────────────────────────────────┤
│ GND          │ GND                                         │
├──────────────┼─────────────────────────────────────────────┤
│ LED          │ GPIO 2  (commande LED IR)                   │
├──────────────┼─────────────────────────────────────────────┤
│ VOUT         │ GPIO 36 (SVP) ← input-only, pas de boot pb  │
└──────────────┴─────────────────────────────────────────────┘
*/

// ─── Calibration ────────────────────────────────────────────────────────────
// Ratio de conversion : ug/m³ par mV (fourni par Waveshare)
#define DUST_COV_RATIO       0.2f

// Tension de base à l'air propre (mV) — à recalibrer selon ton environnement.
// Mesure le voltage brut à l'air propre et ajuste cette valeur.
// #define DUST_NO_DUST_VOLTAGE 400.0f
#define DUST_NO_DUST_VOLTAGE 2320.0f

// Tension système ESP32 en mV
#define DUST_SYS_VOLTAGE     3300.0f

// Résolution ADC ESP32 (12 bits = 4096)
#define DUST_ADC_RESOLUTION  4096.0f

// Taille du buffer de filtrage (moyenne glissante)
#define DUST_FILTER_SIZE     10

// ─── Pins (à adapter selon ton câblage sur le T-Beam) ────────────────────────
// Utilise uniquement GPIO 34/35/36/39 pour l'analogique (input-only, no boot issue)
#define DUST_ILED_PIN        2    // GPIO qui pilote la LED infrarouge du capteur
#define DUST_VOUT_PIN        36   // GPIO36 (SVP) — entrée analogique

// ─── Classe ─────────────────────────────────────────────────────────────────
class DustSensor {
public:
    DustSensor(uint8_t iledPin = DUST_ILED_PIN, uint8_t voutPin = DUST_VOUT_PIN);
 
    // Initialise les pins. À appeler dans setup().
    void begin();
 
    // Effectue une mesure et retourne la densité en ug/m³.
    // Retourne 0.0 si la tension est en dessous du seuil NO_DUST_VOLTAGE.
    float read();
 
    // Retourne le dernier voltage mesuré (mV) — utile pour la calibration.
    float getLastVoltage() const;
 
    // Retourne la dernière densité calculée (ug/m³) sans refaire de mesure.
    float getLastDensity() const;
 
    // Vide le buffer de filtrage (appelé automatiquement après chaque read()).
    void resetBuffer();
 
private:
    uint8_t _iledPin;
    uint8_t _voutPin;
 
    float _lastVoltage;
    float _lastDensity;
 
    // Buffer pour la moyenne glissante
    int _buff[DUST_FILTER_SIZE];
    int _sum;
    bool _initialized;
 
    int _filter(int rawValue);
};
