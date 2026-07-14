#include <Arduino.h>
#include <FindMyAdv.h>

void setup() {
  FindMyAdvConfig tracking;
  tracking.appleAdvertisementKeyBase64 = "";
  tracking.googleAdvertisementEidHex = "";
  tracking.backgroundTask = false;
  FindMyAdv.begin(tracking);
}

void loop() {
  FindMyAdv.poll();
  // Keep the rest of this loop non-blocking and poll at least every 100 ms.
  delay(25);
}

