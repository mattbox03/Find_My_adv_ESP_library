#include <Arduino.h>
#include <FindMyAdv.h>

void setup() {
  FindMyAdvConfig tracking;
  tracking.appleAdvertisementKeyBase64 = ""; // 40 characters, or empty
  tracking.googleAdvertisementEidHex = "";   // 40 hexadecimal characters, or empty
  tracking.advertisingIntervalMs = 2000;
  tracking.txPowerDbm = 0;

  // FindMyAdv runs in its own FreeRTOS task. loop() does not need to call it.
  FindMyAdv.begin(tracking);
}

void loop() {
  // Your existing application remains unchanged.
}

