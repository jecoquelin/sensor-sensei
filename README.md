# sensor-sensei

Early PlatformIO workspace for the `Sensor Sensei` project on ESP32 LoRa boards.

## Current Firmware Targets

- `main`: BMP280 + LoRa node firmware on the T-Beam
- `i2c_scan`: quick I2C address scanner
- `gateway`: Heltec V3 LoRa receiver
- `dust_sensor_demo`: GP2Y1010AU0F dust sensor bring-up on the T-Beam

Build a target with:

```bash
pio run -e dust_sensor_demo
```

The dust sensor work is documented in [docs/gp2y1010-dust-sensor.md](docs/gp2y1010-dust-sensor.md).
