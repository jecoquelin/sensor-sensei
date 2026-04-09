#include <Arduino.h>

#include <Gp2y1010DustSensor.h>

namespace {
constexpr uint8_t kDustSensorAdcPin = 35;
constexpr uint8_t kDustSensorLedDrivePin = 25;
constexpr size_t kReadPulseCount = 25;
constexpr size_t kCalibrationPulseCount = 200;
constexpr uint32_t kReportIntervalMs = 1000;

Gp2y1010DustSensor::Config makeDustSensorConfig() {
    Gp2y1010DustSensor::Config config;
    config.adcPin = kDustSensorAdcPin;
    config.ledDrivePin = kDustSensorLedDrivePin;
    config.outputDividerRatio = 11.0f;
    config.zeroDustVoltageMv = 900.0f;
    config.sensitivityVoltsPer0p1MgPerM3 = 0.5f;
    config.sampleDelayUs = 280;
    config.ledPulseWidthUs = 320;
    config.cyclePeriodUs = 10000;
    return config;
}

Gp2y1010DustSensor dustSensor(makeDustSensorConfig());
uint32_t lastReportMs = 0;

void printHelp() {
    Serial.println("# GP2Y1010AU0F dust sensor demo for T-Beam");
    Serial.println("# Wiring: VCC->3V3, GND->GND, AOUT->GPIO35, ILED->GPIO25");
    Serial.println("# Commands:");
    Serial.println("#   c : calibrate the zero-dust baseline using the current air");
    Serial.println("#   r : restore the default zero-dust baseline from the config");
    Serial.println("#   h : print this help");
    Serial.println("#");
    Serial.println("# density_ug_m3 is an optical dust estimate, not PM1/PM2.5/PM10.");
    Serial.println("# Keep the sensor in clean and stable air before pressing 'c'.");
}

void printCsvHeader() {
    Serial.println("timestamp_ms,module_mv,sensor_mv,zero_mv,density_ug_m3,saturated");
}

void printReading(const Gp2y1010Reading& reading) {
    Serial.print(reading.timestampMs);
    Serial.print(',');
    Serial.print(reading.moduleVoltageMv);
    Serial.print(',');
    Serial.print(reading.sensorVoltageMv, 1);
    Serial.print(',');
    Serial.print(dustSensor.zeroDustVoltageMv(), 1);
    Serial.print(',');
    Serial.print(reading.dustDensityUgPerM3, 1);
    Serial.print(',');
    Serial.println(reading.saturated ? 1 : 0);
}

void calibrateZeroBaseline() {
    Serial.print("# Calibrating zero baseline with ");
    Serial.print(kCalibrationPulseCount);
    Serial.println(" pulses...");

    if (!dustSensor.calibrateZero(kCalibrationPulseCount)) {
        Serial.println("# Calibration failed");
        return;
    }

    Serial.print("# New zero baseline (sensor-side mV): ");
    Serial.println(dustSensor.zeroDustVoltageMv(), 1);
    printCsvHeader();
}

void restoreDefaultZeroBaseline() {
    dustSensor.resetZeroCalibration();
    Serial.print("# Restored zero baseline (sensor-side mV): ");
    Serial.println(dustSensor.zeroDustVoltageMv(), 1);
    printCsvHeader();
}

void handleSerialCommands() {
    while (Serial.available() > 0) {
        const char command = static_cast<char>(Serial.read());

        switch (command) {
            case 'c':
            case 'C':
                calibrateZeroBaseline();
                break;
            case 'r':
            case 'R':
                restoreDefaultZeroBaseline();
                break;
            case 'h':
            case 'H':
            case '?':
                printHelp();
                printCsvHeader();
                break;
            default:
                break;
        }
    }
}
}  // namespace

void setup() {
    Serial.begin(115200);
    delay(1000);

    dustSensor.begin();

    printHelp();
    Serial.print("# Default zero baseline (sensor-side mV): ");
    Serial.println(dustSensor.zeroDustVoltageMv(), 1);
    printCsvHeader();
}

void loop() {
    handleSerialCommands();

    const uint32_t now = millis();
    if (now - lastReportMs < kReportIntervalMs) {
        delay(10);
        return;
    }

    lastReportMs = now;

    Gp2y1010Reading reading;
    if (!dustSensor.read(reading, kReadPulseCount)) {
        Serial.println("# Read failed");
        return;
    }

    printReading(reading);
}
