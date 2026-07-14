#include <Arduino.h>
#include <FindMyAdv.h>

void setup() {
    FindMyAdvConfig config;
    config.appleAdvertisementKeyBase64 =
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA==";
    config.googleAdvertisementEidHex =
        "0000000000000000000000000000000000000000";
    config.backgroundTask = false;
    FindMyAdv.begin(config);
}

void loop() {
    FindMyAdv.poll();
    delay(25);
}
