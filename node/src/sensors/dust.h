#pragma once

#include <Arduino.h>

/*
Branchement Waveshare Dust Sensor (GP2Y1010AU0F / GP2Y1014AU0F)
┌──────────────┬─────────────────────────────────────────────┐
│ Dust Sensor  │                   T-Beam                    │
├──────────────┼─────────────────────────────────────────────┤
│ VCC          │ 5V ← le capteur est conçu pour 5V, pas 3.3V │
├──────────────┼─────────────────────────────────────────────┤
│ GND          │ GND                                         │
├──────────────┼─────────────────────────────────────────────┤
│ LED          │ GPIO 2  (commande de la LED infrarouge)     │
├──────────────┼─────────────────────────────────────────────┤
│ VOUT         │ GPIO 36 (SVP) via pont diviseur 10k/20k     │
│              │ ← GPIO36 est input-only, pas de problème    │
│              │   au boot (pas de niveau imposé au démarrage)│
└──────────────┴─────────────────────────────────────────────┘
*/

// ─── Calibration ─────────────────────────────────────────────────────────────

// Ratio linéaire tension → densité, fourni par Waveshare dans sa documentation.
// Unité : (µg/m³) / mV
#define DUST_COV_RATIO       0.2f

// Tension de sortie à l'air propre (mV). En dessous de ce seuil, le capteur
// est considéré à 0 µg/m³. Valeur mesurée expérimentalement : ~400 mV en air propre.
// Si le capteur retourne 0 en permanence, baisser cette valeur.
#define DUST_NO_DUST_VOLTAGE 400.0f

// Tension de référence de l'ADC ESP32 (3.3V = 3300 mV)
#define DUST_SYS_VOLTAGE     3300.0f

// Résolution ADC 12 bits de l'ESP32 (2^12 = 4096 niveaux, de 0 à 4095)
#define DUST_ADC_RESOLUTION  4096.0f

// Nombre d'échantillons dans la fenêtre glissante du filtre de lissage
#define DUST_FILTER_SIZE     10

// ─── Pins ────────────────────────────────────────────────────────────────────
// Utiliser uniquement GPIO 34/35/36/39 pour l'analogique (entrées seules,
// sans résistance pull-up interne, pas de conflit au boot).
#define DUST_ILED_PIN        2    // GPIO qui pilote la LED infrarouge
#define DUST_VOUT_PIN        36   // GPIO36 (SVP) — entrée analogique uniquement

// ─── Classe ──────────────────────────────────────────────────────────────────
class DustSensor {
public:
    DustSensor(uint8_t iledPin = DUST_ILED_PIN, uint8_t voutPin = DUST_VOUT_PIN);

    // Initialise les GPIO et effectue le warmup de la LED IR (~1.5 s).
    // À appeler dans setup() après que l'alimentation 5V est disponible.
    void begin();

    // Effectue une mesure et retourne la densité en µg/m³.
    // Retourne 0.0 si en dessous du seuil NO_DUST_VOLTAGE (air propre).
    // Retourne -1.0 si la tension lue est nulle (capteur non alimenté).
    float read();

    // Tension brute convertie lors du dernier appel à read() — utile pour la calibration
    float getLastVoltage() const;

    // Densité calculée lors du dernier appel à read() — sans refaire de mesure
    float getLastDensity() const;

    // Réinitialise le buffer de la moyenne glissante (appelé automatiquement si erreur)
    void resetBuffer();

private:
    uint8_t _iledPin;
    uint8_t _voutPin;

    float _lastVoltage;
    float _lastDensity;

    int  _buff[DUST_FILTER_SIZE];  // fenêtre glissante FIFO
    int  _sum;                     // somme courante pour éviter de recalculer à chaque fois
    bool _initialized;             // faux jusqu'au premier appel de _filter()

    int _filter(int rawValue);
};
