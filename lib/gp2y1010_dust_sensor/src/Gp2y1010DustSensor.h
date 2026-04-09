#pragma once

#include <Arduino.h>

struct Gp2y1010Reading {
    uint32_t timestampMs = 0;
    uint32_t moduleVoltageMv = 0;
    float sensorVoltageMv = 0.0f;
    float dustDensityMgPerM3 = 0.0f;
    float dustDensityUgPerM3 = 0.0f;
    bool valid = false;
    bool saturated = false;
};

class Gp2y1010DustSensor {
   public:
    struct Config {
        uint8_t adcPin = 35;
        uint8_t ledDrivePin = 25;
        float outputDividerRatio = 11.0f;
        float zeroDustVoltageMv = 900.0f;
        float sensitivityVoltsPer0p1MgPerM3 = 0.5f;
        uint16_t sampleDelayUs = 280;
        uint16_t ledPulseWidthUs = 320;
        uint16_t cyclePeriodUs = 10000;
#if !defined(ESP32)
        uint16_t fallbackAdcReferenceMv = 1100;
        uint8_t adcResolutionBits = 12;
#endif
    };

    Gp2y1010DustSensor();
    explicit Gp2y1010DustSensor(const Config& config);

    bool begin();
    bool read(Gp2y1010Reading& reading, size_t pulseCount = 20);
    bool calibrateZero(size_t pulseCount = 200);
    void resetZeroCalibration();

    float zeroDustVoltageMv() const;
    const Config& config() const;

   private:
    uint32_t readAveragedModuleVoltageMv(size_t pulseCount);
    uint32_t readSampleModuleVoltageMv();
    uint32_t readAdcMilliVolts() const;
    float moduleToSensorVoltageMv(uint32_t moduleVoltageMv) const;
    float sensorVoltageToDustDensityMgPerM3(float sensorVoltageMv) const;

    Config config_;
    float calibratedZeroDustVoltageMv_;
};
