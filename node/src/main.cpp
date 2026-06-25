/*
 * SensorSensei — Node (LilyGO T-Beam V1.2)
 *
 * Architecture deep sleep :
 *   Toute la logique est dans setup(). Le deep sleep redémarre l'ESP32
 *   depuis le début à chaque réveil, donc loop() n'est jamais atteint.
 *   Les variables en RTC_DATA_ATTR survivent au deep sleep et évitent
 *   de relire l'eFuse ou de re-calibrer à chaque cycle.
 */

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_BMP280.h>
#include <RadioLib.h>
#include <XPowersLib.h>
#include "sensors/dust.h"
#include "../../shared/payload.h"

/*
Branchement BMP280 (HW-611)
┌────────┬────────────────────┐
│ HW-611 │       T-Beam       │
├────────┼────────────────────┤
│ VCC    │ 3.3V               │
├────────┼────────────────────┤
│ GND    │ GND                │
├────────┼────────────────────┤
│ SDA    │ GPIO 21            │
├────────┼────────────────────┤
│ SCL    │ GPIO 22            │
├────────┼────────────────────┤
│ CSB    │ 3.3V ← force le mode I2C, critique │
├────────┼────────────────────┤
│ SDD    │ GND  ← fixe l'adresse I2C à 0x76  │
└────────┴────────────────────┘

Branchement Waveshare Dust Sensor (GP2Y1010AU0F)
┌──────────────┬──────────────────────────────────────────────────┐
│ Dust Sensor  │                     T-Beam                       │
├──────────────┼──────────────────────────────────────────────────┤
│ VCC          │ 5V  ← alimentation directe USB/batterie          │
├──────────────┼──────────────────────────────────────────────────┤
│ GND          │ GND                                              │
├──────────────┼──────────────────────────────────────────────────┤
│ LED          │ GPIO 2  ← pilote la LED infrarouge               │
├──────────────┼──────────────────────────────────────────────────┤
│ VOUT         │ GPIO 36 (ADC0/SVP) via pont diviseur 10k/20k     │
│              │ ← le diviseur ramène 5V → 3.3V pour l'ADC ESP32  │
└──────────────┴──────────────────────────────────────────────────┘

Branchement SX1276 (intégré T-Beam, SPI interne)
┌────────┬────────┐
│ Signal │  GPIO  │
├────────┼────────┤
│ SCK    │ 5      │
│ MISO   │ 19     │
│ MOSI   │ 27     │
│ NSS    │ 18     │
│ RESET  │ 23     │
│ DIO0   │ 26     │
│ DIO1   │ 33     │
└────────┴────────┘
*/

// Durée de veille entre deux mesures. sensor.community accepte des mises à jour
// toutes les 145 s minimum. 5 min donne un bon compromis autonomie / fraîcheur.
#define SLEEP_INTERVAL_US  (5ULL * 60 * 1000000)

// ─── Pins SX1276 (T-Beam V1.2) ───────────────────────────────────────────────
#define LORA_NSS    18
#define LORA_RST    23
#define LORA_DIO0   26
#define LORA_DIO1   33

// ─── Paramètres LoRa 868 MHz ──────────────────────────────────────────────────
#define LORA_FREQ       868.0   // MHz — bande EU
#define LORA_BW         125.0   // kHz
#define LORA_SF         9       // Spreading Factor — compromis portée/débit
#define LORA_CR         7       // Code Rate 4/7
#define LORA_SYNC_WORD  0x12    // 0x12 = réseau LoRa privé (≠ 0x34 TTN public)
#define LORA_POWER      14      // dBm — max légal EU sans licence

// Variables en RTC RAM : survivent au deep sleep, perdues seulement au power cycle.
// Évite de relire l'eFuse MAC (opération lente) à chaque réveil.
RTC_DATA_ATTR static uint32_t DEVICE_ID  = 0;
RTC_DATA_ATTR static bool     first_boot = true;

// ─── Objets matériels ─────────────────────────────────────────────────────────
SX1276         radio = new Module(LORA_NSS, LORA_DIO0, LORA_RST, LORA_DIO1);
Adafruit_BMP280 bmp;
DustSensor     dust(2, 36);
XPowersAXP2101 PMU;
static bool    pmuOk = false;  // faux si PMIC absent ou non reconnu

// ─── Gestion PMIC AXP2101 ─────────────────────────────────────────────────────
static void pmicInit() {
    // PMU.init() initialise Wire en interne — pas besoin de Wire.begin() séparé
    pmuOk = PMU.init(Wire, 21, 22, AXP2101_SLAVE_ADDRESS);
    if (!pmuOk) {
        Serial.println("[PMIC]   Init failed");
        return;
    }
    Serial.println("[PMIC]   AXP2101 OK");

    // Mapping T-Beam V1.2 confirmé par schéma LilyGO :
    // ALDO3 alimente le SX1276, ALDO4 alimente le module GPS (inutilisé ici).
    // Couper ALDO4 économise ~30 mA en permanence.
    PMU.enableALDO3();   // LoRa SX1276 ON
    PMU.disableALDO4();  // GPS OFF
}

// ─── Mise en veille profonde ───────────────────────────────────────────────────
// Cette fonction ne retourne jamais : l'ESP32 redémarre depuis setup() au réveil.
static void enterDeepSleep() {
    // Mettre le radio en veille logicielle avant de couper son alimentation
    // évite des états indéterminés au prochain réveil.
    radio.sleep();

    if (pmuOk)
        PMU.disableALDO3();  // coupe l'alimentation du SX1276

    digitalWrite(2, LOW);  // LED infrarouge du capteur poussière éteinte
    Serial.printf("[NODE]   Deep sleep %llu s\n", SLEEP_INTERVAL_US / 1000000ULL);
    Serial.flush();  // vide le buffer UART avant la coupure
    esp_sleep_enable_timer_wakeup(SLEEP_INTERVAL_US);
    esp_deep_sleep_start();
}

void setup() {
    Serial.begin(115200);

    // Au premier démarrage, on lit le MAC depuis l'eFuse et on le stocke en RTC RAM.
    // Les réveils suivants récupèrent directement la valeur sans accès eFuse.
    if (first_boot) {
        DEVICE_ID  = (uint32_t)(ESP.getEfuseMac() & 0xFFFFFFFF);
        first_boot = false;
        Serial.printf("[NODE]   First boot — Device ID: 0x%08X (%u)\n",
                      DEVICE_ID, DEVICE_ID);
    } else {
        Serial.printf("[NODE]   Wake — Device ID: 0x%08X\n", DEVICE_ID);
    }

    // Initialise le PMIC et les rails d'alimentation
    pmicInit();

    // BMP280 — Wire est déjà initialisé par pmicInit()
    if (!bmp.begin(0x76)) {
        Serial.println("[BMP280] Not found !");
        enterDeepSleep();  // dort quand même pour reprendre au prochain cycle
    }
    Serial.println("[BMP280] OK");

    // SX1276 — ALDO3 est allumé, on peut initialiser RadioLib
    SPI.begin(5, 19, 27, LORA_NSS);
    int state = radio.begin(LORA_FREQ, LORA_BW, LORA_SF, LORA_CR,
                            LORA_SYNC_WORD, LORA_POWER);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("[LoRa]   Init failed: %d\n", state);
        enterDeepSleep();
    }
    Serial.println("[LoRa]   OK");

    // Capteur poussière — begin() inclut le warmup de la LED IR (~1.5 s)
    dust.begin();
    Serial.println("[DUST]   OK");

    // ─── Lecture des capteurs ─────────────────────────────────────────────────
    float temp     = bmp.readTemperature();
    float pressure = bmp.readPressure() / 100.0f;  // Pa → hPa pour la payload
    float pm25     = dust.read();

    Serial.printf("[BMP280] Temp: %.2f C | Pressure: %.2f hPa\n", temp, pressure);
    Serial.printf("[DUST]   PM2.5: %.1f ug/m3 | Voltage: %.0f mV\n",
                  pm25, dust.getLastVoltage());

    // ─── Encodage et transmission ─────────────────────────────────────────────
    SensorPayload p = { DEVICE_ID, temp, pressure, pm25 };
    uint8_t payload[PAYLOAD_LEN];
    encodePayload(payload, p);

    state = radio.transmit(payload, sizeof(payload));
    if (state == RADIOLIB_ERR_NONE)
        Serial.printf("[LoRa]   Sent %d bytes OK\n", sizeof(payload));
    else
        Serial.printf("[LoRa]   TX error: %d\n", state);

    // ─── Retour en veille profonde ────────────────────────────────────────────
    enterDeepSleep();
}

void loop() {
    // Jamais atteint — setup() se termine toujours par enterDeepSleep()
}
