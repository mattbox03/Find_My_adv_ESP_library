#include <Arduino.h>
#include <FindMyAdv.h>

void setup() {
    FindMyAdvConfig config;
    config.appleAdvertisementKeyBase64 = "";
    config.googleAdvertisementEidHex = "";
    config.backgroundTask = false;
    FindMyAdv.begin(config);
}

void loop() {
    FindMyAdv.poll();
    delay(25);
}

