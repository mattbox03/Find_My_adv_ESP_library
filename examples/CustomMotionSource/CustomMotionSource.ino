#include <Arduino.h>
#include <FindMyAdv.h>

constexpr uint8_t MOTION_PIN = 4;

bool movementDetected() {
  // Replace this with any IMU, vibration switch, GPIO, or application state.
  return digitalRead(MOTION_PIN) == LOW;
}

void setup() {
  pinMode(MOTION_PIN, INPUT_PULLUP);

  FindMyAdvConfig tracking;
  tracking.appleAdvertisementKeyBase64 = "";
  tracking.googleAdvertisementEidHex = "";
  tracking.advertisingIntervalMs = 2000;
  tracking.motionIntervalMs = 200;
  tracking.motionHoldMs = 30000;
  tracking.motionDetectedCallback = movementDetected;
  FindMyAdv.begin(tracking);
}

void loop() {
  // Existing application code.
}

