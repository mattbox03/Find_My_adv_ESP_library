#pragma once

#include <Arduino.h>
#include <Wire.h>

struct FindMyAdvConfig {
    const char *appleAdvertisementKeyBase64 = "";
    const char *googleAdvertisementEidHex = "";
    uint32_t advertisingIntervalMs = 2000;
    int8_t txPowerDbm = 0;
    uint32_t providerWindowMs = 5000;

    bool accelerometerEnabled = false;
    TwoWire *accelerometerWire = &Wire;
    bool initializeAccelerometerBus = true;
    int8_t accelerometerSda = 8;
    int8_t accelerometerScl = 9;
    uint8_t accelerometerAddress = 0; // 0 = probe 0x18 and 0x19
    uint16_t motionThreshold = 320;
    uint32_t motionIntervalMs = 200;
    uint32_t motionHoldMs = 30000;
    // Optional custom motion source. Return true when movement is detected.
    // It can replace or complement the built-in ST accelerometer driver.
    bool (*motionDetectedCallback)() = nullptr;

    // true: no FindMyAdv call is needed from loop(). false: call poll() often.
    bool backgroundTask = true;
};

enum class FindMyAdvStatus : uint8_t {
    Stopped,
    Running,
    NoValidKey,
    BleInitializationFailed,
    TaskCreationFailed,
};

class FindMyAdvClass {
public:
    // Safe to call once from setup(). The configuration is copied internally.
    bool begin(const FindMyAdvConfig &config);
    // Replaces keys and radio settings while the firmware is running.
    bool reconfigure(const FindMyAdvConfig &config);
    bool setKeys(const char *appleAdvertisementKeyBase64,
                 const char *googleAdvertisementEidHex);
    void end();

    // Only required when config.backgroundTask is false.
    void poll();

    bool isRunning() const;
    bool isAppleEnabled() const;
    bool isGoogleEnabled() const;
    bool isAccelerometerDetected() const;
    bool isMotionModeActive() const;
    uint32_t activeAdvertisingIntervalMs() const;
    FindMyAdvStatus status() const;
    const char *lastError() const;
};

extern FindMyAdvClass FindMyAdv;
