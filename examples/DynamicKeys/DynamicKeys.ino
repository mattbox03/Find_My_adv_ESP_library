#include <Arduino.h>
#include <Preferences.h>
#include <FindMyAdv.h>

Preferences settings;
String appleKey;
String googleEid;

void applyStoredKeys() {
  appleKey = settings.getString("apple", "");
  googleEid = settings.getString("google", "");

  if (FindMyAdv.isRunning()) {
    FindMyAdv.setKeys(appleKey.c_str(), googleEid.c_str());
    return;
  }

  FindMyAdvConfig tracking;
  tracking.appleAdvertisementKeyBase64 = appleKey.c_str();
  tracking.googleAdvertisementEidHex = googleEid.c_str();
  FindMyAdv.begin(tracking);
}

void setup() {
  settings.begin("findmyadv", false);
  applyStoredKeys();

  // A web/MQTT handler can update values in the same way:
  // settings.putString("apple", newAppleKey);
  // settings.putString("google", newGoogleEid);
  // FindMyAdv.setKeys(newAppleKey.c_str(), newGoogleEid.c_str());
}

void loop() {
  // Existing application code.
}

