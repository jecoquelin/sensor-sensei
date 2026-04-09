#include "Gp2y1010DustSensor.h"

Gp2y1010DustSensor::Gp2y1010DustSensor() : Gp2y1010DustSensor(Config()) {}

Gp2y1010DustSensor::Gp2y1010DustSensor(const Config& config)
    : config_(config), calibratedZeroDustVoltageMv_(config.zeroDustVoltageMv) {}

bool Gp2y1010DustSensor::begin() {
    pinMode(config_.ledDrivePin, OUTPUT);
    digitalWrite(config_.ledDrivePin, LOW);
    pinMode(config_.adcPin, INPUT);

#if defined(ESP32)
    analogReadResolution(12);
    analogSetPinAttenuation(config_.adcPin, ADC_0db);
#endif

    return true;
}

bool Gp2y1010DustSensor::read(Gp2y1010Reading& reading, size_t pulseCount) {
    if (pulseCount == 0) {
        return false;
    }

    const uint32_t moduleVoltageMv = readAveragedModuleVoltageMv(pulseCount);
    const float sensorVoltageMv = moduleToSensorVoltageMv(moduleVoltageMv);
    const float dustDensityMgPerM3 = sensorVoltageToDustDensityMgPerM3(sensorVoltageMv);

    reading.timestampMs = millis();
    reading.moduleVoltageMv = moduleVoltageMv;
    reading.sensorVoltageMv = sensorVoltageMv;
    reading.dustDensityMgPerM3 = dustDensityMgPerM3;
    reading.dustDensityUgPerM3 = dustDensityMgPerM3 * 1000.0f;
    reading.valid = true;
    reading.saturated = reading.dustDensityUgPerM3 >= 500.0f;

    return true;
}

bool Gp2y1010DustSensor::calibrateZero(size_t pulseCount) {
    if (pulseCount == 0) {
        return false;
    }

    const uint32_t moduleVoltageMv = readAveragedModuleVoltageMv(pulseCount);
    calibratedZeroDustVoltageMv_ = moduleToSensorVoltageMv(moduleVoltageMv);
    return true;
}

void Gp2y1010DustSensor::resetZeroCalibration() {
    calibratedZeroDustVoltageMv_ = config_.zeroDustVoltageMv;
}

float Gp2y1010DustSensor::zeroDustVoltageMv() const {
    return calibratedZeroDustVoltageMv_;
}

const Gp2y1010DustSensor::Config& Gp2y1010DustSensor::config() const {
    return config_;
}

uint32_t Gp2y1010DustSensor::readAveragedModuleVoltageMv(size_t pulseCount) {
    uint64_t totalModuleVoltageMv = 0;

    for (size_t i = 0; i < pulseCount; ++i) {
        totalModuleVoltageMv += readSampleModuleVoltageMv();
    }

    return static_cast<uint32_t>(totalModuleVoltageMv / pulseCount);
}

uint32_t Gp2y1010DustSensor::readSampleModuleVoltageMv() {
    // Follow the GP2Y1010 timing: LED on, wait 280 us, sample, then finish the 10 ms cycle.
    digitalWrite(config_.ledDrivePin, HIGH);
    delayMicroseconds(config_.sampleDelayUs);

    const uint32_t moduleVoltageMv = readAdcMilliVolts();

    if (config_.ledPulseWidthUs > config_.sampleDelayUs) {
        delayMicroseconds(config_.ledPulseWidthUs - config_.sampleDelayUs);
    }

    digitalWrite(config_.ledDrivePin, LOW);

    if (config_.cyclePeriodUs > config_.ledPulseWidthUs) {
        delayMicroseconds(config_.cyclePeriodUs - config_.ledPulseWidthUs);
    }

    return moduleVoltageMv;
}

uint32_t Gp2y1010DustSensor::readAdcMilliVolts() const {
#if defined(ESP32)
    return analogReadMilliVolts(config_.adcPin);
#else
    const uint32_t adcMaxValue = (1UL << config_.adcResolutionBits) - 1UL;
    return (static_cast<uint32_t>(analogRead(config_.adcPin)) * config_.fallbackAdcReferenceMv) /
           adcMaxValue;
#endif
}

float Gp2y1010DustSensor::moduleToSensorVoltageMv(uint32_t moduleVoltageMv) const {
    return static_cast<float>(moduleVoltageMv) * config_.outputDividerRatio;
}

float Gp2y1010DustSensor::sensorVoltageToDustDensityMgPerM3(float sensorVoltageMv) const {
    const float deltaVoltageMv = sensorVoltageMv - calibratedZeroDustVoltageMv_;
    if (deltaVoltageMv <= 0.0f) {
        return 0.0f;
    }

    const float deltaVoltageV = deltaVoltageMv / 1000.0f;
    return (deltaVoltageV / config_.sensitivityVoltsPer0p1MgPerM3) * 0.1f;
}
