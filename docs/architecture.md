# Architecture — SensorSensei

## Vue d'ensemble

```
┌──────────────────────────────────────────────────────────────┐
│                         Node                                  │
│                    (LilyGO T-Beam)                            │
│                                                               │
│  BMP280 ──────────► ESP32 ──────────► SX1276 ─────────────►  │
│  (temp/pression)    (encode payload)   (868 MHz LoRa)         │
│  GP2Y1010 ─────────►                                          │
│  (poussière PM2.5)                                            │
│                         [deep sleep entre mesures]            │
└──────────────────────────────────────────────────────────────┘
                              │
                         868 MHz LoRa
                              │
                              ▼
┌──────────────────────────────────────────────────────────────┐
│                        Gateway                                │
│                   (Heltec WiFi LoRa 32 V3)                    │
│                                                               │
│  SX1262 ──────────► ESP32 ──────────► WiFi ─────────────────►│
│  (réception LoRa)   (décode payload)                          │
│                                                               │
└──────────────────────────────────────────────────────────────┘
                              │
                            HTTPS
                              │
                              ▼
                    api.sensor.community
```

## Composants

| Rôle    | Carte                   | MCU   | LoRa   | Alimentation |
|---------|-------------------------|-------|--------|--------------|
| Node    | LilyGO T-Beam V1.2      | ESP32 | SX1276 | Batterie LiPo |
| Gateway | Heltec WiFi LoRa 32 V3  | ESP32 | SX1262 | USB           |

## Paramètres LoRa

| Paramètre     | Valeur | Remarque                              |
|---------------|--------|---------------------------------------|
| Fréquence     | 868 MHz | Bande EU ISM                         |
| Bandwidth     | 125 kHz |                                      |
| Spreading Factor | SF9  | Compromis portée / débit             |
| Code Rate     | 4/7    |                                       |
| Sync Word     | 0x12   | Réseau privé (≠ 0x34 TTN public)     |
| Puissance TX  | 14 dBm | Maximum légal EU sans licence        |
| Taille payload | 10 bytes | Sous la limite 51 bytes SF9/125kHz |

## Format de la payload

La payload est encodée en big-endian sur **10 bytes** pour minimiser le temps d'émission LoRa.

```
byte 0-3 : device_id    — uint32 big-endian (32 bits bas du MAC ESP32)
byte 4-5 : température  — int16 big-endian, °C × 100   (ex: 21.5°C → 2150)
byte 6-7 : pression     — uint16 big-endian, hPa entier (ex: 1013 hPa → 1013)
byte 8-9 : PM2.5        — uint16 big-endian, µg/m³ × 10 (ex: 12.5 → 125)
```

Les floats sont convertis en entiers pour éviter IEEE 754 sur 4 bytes par champ.
La structure C++ `SensorPayload` et les fonctions `encodePayload`/`decodePayload` sont
partagées entre le node et le gateway via `shared/payload.h`.

## Cycle de vie du node

```
Réveil
  │
  ├─► Lecture MAC (eFuse) — uniquement au premier démarrage
  │   Les réveils suivants récupèrent le device_id depuis la RTC RAM
  │
  ├─► PMIC AXP2101 — active ALDO3 (SX1276), désactive ALDO4 (GPS)
  │
  ├─► Lecture BMP280 (I2C)
  ├─► Lecture GP2Y1010 (ADC)
  │
  ├─► Encodage payload (10 bytes)
  ├─► Transmission LoRa
  │
  └─► Deep sleep 5 minutes
        (ALDO3 coupé → SX1276 hors tension pendant le sommeil)
```

## Pattern Gatekeeper

Le gateway ne transmet **pas aveuglément** les paquets reçus à sensor.community.
Tout paquet LoRa passe d'abord par un **gatekeeper** qui applique deux niveaux de validation :

```
Paquet LoRa reçu
        │
        ▼
┌───────────────────┐
│  1. Whitelist     │  Le device_id est-il dans la liste des nodes autorisés ?
│     device_id     │──── non ──► paquet rejeté, log "[GK] Device non autorisé"
└───────────────────┘
        │ oui
        ▼
┌───────────────────┐
│  2. Plages        │  Les valeurs capteurs sont-elles physiquement plausibles ?
│     valides       │──── non ──► paquet rejeté, log "[GK] hors plage"
└───────────────────┘
        │ oui
        ▼
  Envoi sensor.community
```

**Plages de validation BMP280 :**

| Grandeur    | Min      | Max      | Référence                          |
|-------------|----------|----------|------------------------------------|
| Température | -40 °C   | +85 °C   | Limites opérationnelles du BMP280  |
| Pression    | 300 hPa  | 1100 hPa | Limites opérationnelles du BMP280  |

**Pourquoi ce pattern :**
- **Sécurité** : sans whitelist, n'importe quel node LoRa sur 868 MHz avec le bon sync word
  pourrait injecter des données dans la station sensor.community du gateway.
- **Qualité des données** : un capteur défaillant ou mal câblé peut retourner des valeurs
  aberrantes (0 hPa, 999 °C). Le gatekeeper empêche ces données d'atteindre la communauté.
- **Mode dev vs prod** : si `AUTHORIZED_DEVICES` est vide, tous les nodes sont acceptés
  (pratique pendant le développement). En production, remplir la whitelist dans `gatekeeper.h`.

## Envoi vers sensor.community

Le gateway envoie deux requêtes HTTPS POST par paquet LoRa reçu :

| Pin | Capteur   | Champs envoyés              |
|-----|-----------|-----------------------------|
| 11  | BMP280    | `temperature`, `pressure`   |
| 1   | SDS011*   | `P1` (PM10), `P2` (PM2.5)  |

*Le GP2Y1010 ne distingue pas PM10/PM2.5 — la même valeur est envoyée pour P1 et P2.

L'identifiant de station est `esp32-<device_id_decimal>`.
