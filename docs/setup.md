# Guide de démarrage — SensorSensei

## Prérequis

- [PlatformIO](https://platformio.org/) (extension VS Code ou CLI)
- Python 3.x (requis par PlatformIO)
- Deux cartes :
  - LilyGO T-Beam V1.2 (node)
  - Heltec WiFi LoRa 32 V3 (gateway)

## Structure du dépôt

```
SensorSensei/
├── node/           # Firmware du node (T-Beam)
│   ├── src/
│   │   ├── main.cpp
│   │   ├── sensors/
│   │   │   ├── dust.cpp / dust.h     # GP2Y1010 (poussière)
│   │   │   └── bmp280.cpp / bmp280.h # BMP280 (temp/pression)
│   │   └── lora/
│   │       └── transmitter.cpp / .h  # Émetteur RadioLib
│   └── platformio.ini
├── gateway/        # Firmware du gateway (Heltec V3)
│   ├── src/
│   │   ├── main.cpp
│   │   ├── config.h                  # WiFi SSID/password
│   │   ├── gatekeeper.cpp / .h       # Whitelist des nodes autorisés
│   │   ├── display/                  # Écran OLED
│   │   ├── lora/
│   │   │   └── receiver.cpp / .h     # Récepteur RadioLib
│   │   └── http/
│   │       └── sensor_community.cpp / .h  # API sensor.community
│   └── platformio.ini
├── shared/
│   └── payload.h   # Format de payload partagé node ↔ gateway
└── docs/
```

## Configuration

### WiFi (gateway uniquement)

Modifier `gateway/src/config.h` :

```cpp
#define WIFI_SSID     "MonReseau"
#define WIFI_PASSWORD "MonMotDePasse"
```

### Whitelist des nodes (optionnel)

Par défaut, le gateway accepte tous les nodes (mode développement).
Pour restreindre aux nodes connus, renseigner `gateway/src/gatekeeper.h` :

```cpp
static const uint32_t AUTHORIZED_DEVICES[] = {
    0xABCD1234,  // Device ID du T-Beam 1
    0x5678EFGH,  // Device ID du T-Beam 2
};
```

Le Device ID s'affiche au démarrage du node dans les logs série :
```
[NODE]   First boot — Device ID: 0xABCD1234 (2882343476)
```

## Compilation et flash

### Node (T-Beam)

```bash
cd node
pio run -t upload        # compile + flash
pio device monitor       # ouvre le port série (115200 baud)
```

### Gateway (Heltec V3)

```bash
cd gateway
pio run -t upload
pio device monitor
```

## Logs série attendus

### Node

```
[NODE]   First boot — Device ID: 0xABCD1234 (2882343476)
[PMIC]   AXP2101 OK
[BMP280] OK
[LoRa]   OK
[DUST]   Warming up...
[DUST]   OK
[BMP280] Temp: 22.45 C | Pressure: 1012.00 hPa
[DUST]   PM2.5: 8.3 ug/m3 | Voltage: 640 mV
[LoRa]   Sent 10 bytes OK
[NODE]   Deep sleep 300 s
```

### Gateway

```
[LoRa] Gateway ready — écoute sur 868 MHz
─────────────────────────────
Device ID  : 0xABCD1234
Température: 22.45 °C
Pression   : 1012 hPa
Poussière  : 8.3 µg/m³
RSSI       : -87.5 dBm
SNR        : 7.2 dB
─────────────────────────────
[SC] pin 3  → OK (201)
[SC] pin 1  → OK (201)
```

## Bibliothèques utilisées

| Bibliothèque              | Version | Usage                          |
|---------------------------|---------|--------------------------------|
| RadioLib                  | ^6.6.0  | SX1276 (node) / SX1262 (gateway) |
| Adafruit BMP280 Library   | ^3.0.0  | Capteur température/pression   |
| Adafruit Unified Sensor   | ^1.1.15 | Dépendance Adafruit            |
| XPowersLib                | ^0.2.4  | PMIC AXP2101 (T-Beam)          |
| Adafruit SSD1306          | ^2.5.7  | Écran OLED (Heltec)            |
| Adafruit GFX Library      | ^1.11.9 | Dépendance SSD1306             |
