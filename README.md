# FindMyAdv

Reusable Apple Find My and Google Find Hub BLE advertising for Arduino on
BLE-capable ESP32 chips. Add one `begin()` call to an existing firmware; the
library runs its scheduler in a private FreeRTOS task and does not require code
in `loop()`.

`begin()` may be called from `setup()` or later, after the host application has
loaded credentials or completed its own initialization. `end()` stops the
module, and a later `begin()` starts it again.

FindMyAdv is an unofficial, open-source interoperability project. It is not
affiliated with or endorsed by Apple, Google, or Espressif.

Italian documentation: [`docs/README.it.md`](docs/README.it.md)  
Complete API reference: [`docs/API.md`](docs/API.md)

Additional references:

- [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md)
- [`docs/TROUBLESHOOTING.md`](docs/TROUBLESHOOTING.md)
- [`docs/RELEASING.md`](docs/RELEASING.md)
- [`CONTRIBUTING.md`](CONTRIBUTING.md)
- [`SECURITY.md`](SECURITY.md)

## Supported hardware

| Chip | Bluetooth | Backend | Status |
| --- | --- | --- | --- |
| ESP32 | BLE 4.2 | Bluedroid | Supported |
| ESP32-C3 | BLE 5 | Bluedroid | Supported |
| ESP32-S3 | BLE 5 | Bluedroid | Supported |
| ESP32-C6 | BLE 5 | NimBLE | Supported |
| ESP32-C2 | BLE 5 | Selected from Arduino core | Compatible; not yet in the release build matrix |
| ESP32-C5 / ESP32-C61 | BLE | Selected from Arduino core | Compatible; requires a supporting Arduino core |
| ESP32-H2 | BLE 5 | NimBLE | Compatible; build with a supporting Arduino core |
| ESP8266 / ESP32-S2 / ESP32-P4 | No Bluetooth radio | — | Not possible |

The current standalone firmware validates the ESP32, C3, S3, and C6 targets.
Backend selection uses `CONFIG_BT_NIMBLE_ENABLED` or
`CONFIG_BT_BLUEDROID_ENABLED`, rather than a hard-coded chip list. New ESP32
variants therefore work when their Arduino core exposes one of those supported
BLE hosts.

## Installation

### PlatformIO project

Install the library directly from its public GitHub repository:

```ini
lib_deps = https://github.com/mattbox03/Find_My_adv_ESP_library.git
```

For a local checkout, copy the complete `FindMyAdv` directory into the
project's `lib` directory:

```text
your-project/
|-- lib/
|   `-- FindMyAdv/
|       |-- library.json
|       `-- src/
`-- src/
    `-- main.cpp
```

### Arduino IDE

Put the `FindMyAdv` directory in the Arduino libraries directory, or create a
ZIP whose root directory is named `FindMyAdv` and use **Sketch > Include
Library > Add .ZIP Library**.

## Minimal integration

```cpp
#include <Arduino.h>
#include <FindMyAdv.h>

void setup() {
    FindMyAdvConfig tracking;
    tracking.appleAdvertisementKeyBase64 = "APPLE_BASE64_KEY_OR_EMPTY";
    tracking.googleAdvertisementEidHex = "GOOGLE_40_HEX_EID_OR_EMPTY";
    tracking.advertisingIntervalMs = 2000;
    tracking.txPowerDbm = 0;

    if (!FindMyAdv.begin(tracking)) {
        Serial.println(FindMyAdv.lastError());
    }
}

void loop() {
    // Existing application code: FindMyAdv needs no calls here.
}
```

Either key may be empty. With both keys configured, the library time-slices
the Apple and Google legacy advertising frames. With one key, it advertises
only that provider.

## Change keys or settings at runtime

The library copies all input strings, so temporary `String` values and values
read from a web form, MQTT, Preferences/NVS, or a configuration file are safe:

```cpp
String appleFromWeb = request->arg("apple_key");
String googleFromWeb = request->arg("google_eid");

if (!FindMyAdv.setKeys(appleFromWeb.c_str(), googleFromWeb.c_str())) {
    Serial.println(FindMyAdv.lastError());
}
```

To change intervals, TX power, accelerometer settings, and keys together:

```cpp
FindMyAdvConfig updated;
updated.appleAdvertisementKeyBase64 = appleFromWeb.c_str();
updated.googleAdvertisementEidHex = googleFromWeb.c_str();
updated.advertisingIntervalMs = 1000;
updated.txPowerDbm = 3;
FindMyAdv.reconfigure(updated);
```

Persistence deliberately remains under application control. Save the values
with `Preferences`, your existing settings manager, or another secure storage
mechanism, then pass them to `begin()` at boot. This avoids imposing a Wi-Fi
portal, filesystem, network service, or credential format on the host firmware.

## Optional motion-based advertising

LIS3DH, LIS2DH, and LIS2DH12 accelerometers are supported at I2C address `0x18`
or `0x19`. Set address `0` to probe both automatically:

```cpp
tracking.accelerometerEnabled = true;
tracking.accelerometerWire = &Wire; // use &Wire1 for a separate application bus
tracking.initializeAccelerometerBus = true;
tracking.accelerometerSda = 8;
tracking.accelerometerScl = 9;
tracking.accelerometerAddress = 0;
tracking.motionIntervalMs = 200;
tracking.motionHoldMs = 30000;
tracking.motionThreshold = 320;
```

If the host firmware already initialized that I2C bus, set
`initializeAccelerometerBus = false` so the library does not change its pins or
clock configuration.

Any other motion sensor is supported through a callback, without modifying the
library:

```cpp
bool moved() {
    return myImu.motionInterruptActive();
}

tracking.motionDetectedCallback = moved;
```

## Integration limits

- Wi-Fi, sensors, displays, MQTT, web servers, and normal application tasks can
  run alongside FindMyAdv.
- The library owns the controller's legacy BLE advertising while running. An
  application that already advertises a BLE GATT service must coordinate that
  advertising explicitly; two independent modules cannot transparently own the
  same legacy advertising set.
- BLE connections may interrupt or alter advertising depending on the chip,
  controller resources, and application stack.
- No radio can advertise during ESP deep sleep. Stop the library before sleep
  and call `begin()` again after wake-up.
- Advertising two providers is time-slicing, not two simultaneous legacy radio
  packets. `providerWindowMs` controls the rotation window.

## Security

Apple advertisement keys and Google EIDs identify a tracker. Do not publish
real keys in source control, logs, examples, issue reports, or public binaries.
The library never sends keys over Wi-Fi and does not persist them by itself.

## Examples

- `Minimal`: one-call integration.
- `DynamicKeys`: load keys from Preferences and replace them at runtime.
- `ManualPolling`: disable the FreeRTOS scheduler task and call `poll()` from
  an existing cooperative loop.
- `CustomMotionSource`: use any IMU, GPIO, or application event for dynamic
  advertising speed.

## License

MIT. See [`LICENSE`](LICENSE).
