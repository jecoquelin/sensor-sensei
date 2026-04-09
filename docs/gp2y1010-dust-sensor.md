# GP2Y1010AU0F Dust Sensor Bring-Up

This repository now contains a first local driver and a serial demo for the Waveshare Dust Sensor `SKU:10500`, based on the Sharp `GP2Y1010AU0F`.

## What This Sensor Provides

- Analog optical dust sensing
- Detection of fine particles above `0.8 um`
- An estimated dust density in `ug/m3`
- A custom signal path that is not directly compatible with `Sensor.Community` PM sensors such as `SDS011`, `PMSxxxx`, or `SPS30`

The driver computes an estimated dust density from the module output. It does not output real `PM1`, `PM2.5`, or `PM10` buckets.

The current estimate uses:

- Waveshare module output voltage, compensated with a divider ratio of `11`
- a default zero-dust baseline of `900 mV` on the sensor side
- the Sharp typical sensitivity of `0.5 V` per `0.1 mg/m3`

## Wiring On T-Beam

- `VCC -> 3V3`
- `GND -> GND`
- `AOUT -> GPIO35`
- `ILED -> GPIO25`

`GPIO35` is an `ADC1` pin on ESP32, which avoids the usual `ADC2` and Wi-Fi contention.

## Build And Flash

Build the dedicated firmware with:

```bash
pio run -e dust_sensor_demo
```

Flash it with:

```bash
pio run -e dust_sensor_demo -t upload
```

Open the serial monitor with:

```bash
pio device monitor -b 115200
```

## Serial Output

The demo prints CSV rows with:

```text
timestamp_ms,module_mv,sensor_mv,zero_mv,density_ug_m3,saturated
```

Field meaning:

- `module_mv`: measured module output in millivolts
- `sensor_mv`: estimated sensor-side voltage after compensating the divider ratio used by the Waveshare board
- `zero_mv`: current zero-dust baseline used by the driver
- `density_ug_m3`: estimated dust density
- `saturated`: `1` when the estimate reaches the module range limit around `500 ug/m3`

## Serial Commands

- `c`: calibrate the zero-dust baseline from the current air
- `r`: restore the default zero-dust baseline from the config
- `h`: print the help again

Calibrate only in clean and stable air.

## Driver Notes

The low-level sampling sequence follows the module documentation:

- drive `ILED` high
- wait `280 us`
- sample `AOUT`
- keep the pulse width near `320 us`
- wait until a `10 ms` cycle is complete before the next pulse

The current implementation is in:

- `lib/gp2y1010_dust_sensor/src/Gp2y1010DustSensor.h`
- `lib/gp2y1010_dust_sensor/src/Gp2y1010DustSensor.cpp`
- `src/dust_sensor_demo.cpp`

## Validation Checklist

- Let the sensor stabilize for a few minutes after power-on
- Log ambient values in still air
- Run one controlled perturbation close to the air inlet
- Check that the value rises and then returns near baseline
- Re-run with a baseline calibration if the ambient offset stays too high

## References

- Waveshare user manual: `https://files.waveshare.com/upload/0/0a/Dust-Sensor-User-Manual-EN.pdf`
- Sharp datasheet: `https://files.waveshare.com/upload/d/dd/GP2Y1010AU0F.pdf`
- Arduino-ESP32 ADC docs: `https://docs.espressif.com/projects/arduino-esp32/en/latest/api/adc.html`
