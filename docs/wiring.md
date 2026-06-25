# Câblage — SensorSensei Node (LilyGO T-Beam V1.2)

## BMP280 (HW-611) — I2C

| HW-611 | T-Beam | Remarque |
|--------|--------|----------|
| VCC    | 3.3V   |          |
| GND    | GND    |          |
| SDA    | GPIO 21 |         |
| SCL    | GPIO 22 |         |
| CSB    | 3.3V   | Force le mode I2C (critique) |
| SDD    | GND    | Fixe l'adresse I2C à 0x76   |

## Capteur de poussière GP2Y1010AU0F (Waveshare) — ADC

| Dust Sensor | T-Beam           | Remarque |
|-------------|------------------|----------|
| VCC         | 5V (USB/batterie) | Alimentation directe |
| GND         | GND              |          |
| LED         | GPIO 2           | Pilote la LED infrarouge |
| VOUT        | GPIO 36 (ADC0/SVP) via pont diviseur 10k/20k | Le diviseur ramène 5V → 3.3V pour l'ADC ESP32 |

### Schéma du pont diviseur de tension

```
VOUT capteur (0–5V)
        │
       10kΩ
        │
        ├──────── GPIO 36 (ADC ESP32, 0–3.3V)
        │
       20kΩ
        │
       GND
```

La tension VOUT du capteur peut atteindre 5V. Le pont diviseur (10kΩ/20kΩ) ramène
la tension dans la plage 0–3.3V de l'ADC ESP32 avec un rapport 2/3.

## SX1276 LoRa — SPI interne T-Beam

Le SX1276 est intégré à la carte T-Beam et câblé en interne :

| Signal | GPIO |
|--------|------|
| SCK    | 5    |
| MISO   | 19   |
| MOSI   | 27   |
| NSS    | 18   |
| RESET  | 23   |
| DIO0   | 26   |
| DIO1   | 33   |

L'alimentation du SX1276 est contrôlée par le PMIC AXP2101 via le rail **ALDO3**.
Il est mis hors tension pendant le deep sleep pour économiser la batterie.

---

# Câblage — SensorSensei Gateway (Heltec WiFi LoRa 32 V3)

## SX1262 LoRa — SPI interne Heltec

Le SX1262 est intégré à la carte Heltec V3 et câblé en interne :

| Signal | GPIO |
|--------|------|
| SCK    | 9    |
| MISO   | 11   |
| MOSI   | 10   |
| NSS    | 8    |
| RESET  | 12   |
| DIO1   | 14   |
| BUSY   | 13   |

## Écran OLED SSD1306

L'écran OLED 0.96" est intégré à la Heltec V3 et géré via la bibliothèque
Adafruit SSD1306 (I2C interne).
