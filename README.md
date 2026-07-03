# SensorSensei

Réseau de capteurs IoT low-power basé sur LoRa 868 MHz, compatible [sensor.community](https://sensor.community/).

## Architecture

- **Node** : LilyGO T-Beam V1.2 (ESP32 + SX1276) — mesure température, pression et PM2.5, transmet via LoRa, repart en deep sleep
- **Gateway** : Heltec WiFi LoRa 32 V3 (ESP32 + SX1262) — reçoit les paquets LoRa et les transmet à l'API sensor.community via WiFi

## Documentation

- [Architecture et format de payload](docs/architecture.md)
- [Câblage des capteurs](docs/wiring.md)
- [Guide de démarrage](docs/setup.md)

## Stack technique

- PlatformIO / Arduino framework
- C++ / ESP32
- RadioLib (SX1276 + SX1262)
- Deep sleep — intervalle de mesure configurable (défaut : 5 min)
