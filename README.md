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
| ESP32 | BLE 4.2 | NimBLE-Arduino | Supported |
| ESP32-C3 | BLE 5 | NimBLE-Arduino | Supported |
| ESP32-S3 | BLE 5 | NimBLE-Arduino | Supported |
| ESP32-C6 | BLE 5 | NimBLE-Arduino | Supported |
| ESP32-C2 | BLE 5 | NimBLE-Arduino | Compatible; not yet in the release build matrix |
| ESP32-C5 / ESP32-C61 | BLE | NimBLE-Arduino | Compatible; requires a supporting Arduino core |
| ESP32-H2 | BLE 5 | NimBLE-Arduino | Compatible; build with a supporting Arduino core |
| ESP8266 / ESP32-S2 / ESP32-P4 | No Bluetooth radio | — | Not possible |

The current standalone firmware validates the ESP32, C3, S3, and C6 targets.
The default backend uses NimBLE-Arduino and calls its raw GAP advertiser
directly. The higher-level scanner, client, server, security, and advertising
object APIs are not used. A legacy Bluedroid compatibility backend remains
available through `FINDMYADV_USE_BLUEDROID=1` for applications already tied to
that host, at a substantially higher memory cost.

## Installation

### PlatformIO project

Install the library directly from its public GitHub repository:

```ini
lib_deps = https://github.com/mattbox03/Find_My_adv_ESP_library.git
```

That is the only project dependency to add. The FindMyAdv manifest resolves
its single external dependency, `NimBLE-Arduino >= 2.5.0`, automatically. Do
not also install the old Arduino `ESP32 BLE Arduino` library.

ESP32, ESP32-C3, and ESP32-S3 can use the stable PlatformIO platform:

```ini
[env:esp32c3]
platform = espressif32@7.0.1
board = esp32-c3-devkitm-1
framework = arduino
lib_deps = https://github.com/mattbox03/Find_My_adv_ESP_library.git
```

ESP32-C6 needs an Arduino-ESP32 3.x platform. This configuration is tested:

```ini
[env:esp32c6]
platform = https://github.com/pioarduino/platform-espressif32/releases/download/55.03.39/platform-espressif32.zip
platform_packages =
    framework-arduinoespressif32 @ https://github.com/espressif/arduino-esp32/releases/download/3.3.9/esp32-core-3.3.9.tar.xz
board = esp32-c6-devkitc-1
framework = arduino
lib_deps = https://github.com/mattbox03/Find_My_adv_ESP_library.git
```

On a Windows machine that has already installed Arduino-ESP32 2.x, PlatformIO
may reuse that same-named framework package instead of the C6 override. Build
the C6 environment with an isolated package directory to avoid the collision:

```powershell
$env:PLATFORMIO_PACKAGES_DIR="$PWD\.pio-packages-c6"
pio run -e esp32c6
```

The first isolated build downloads the complete C6 toolchain and can take
several minutes. Later builds reuse `.pio-packages-c6`. See
[`docs/TROUBLESHOOTING.md`](docs/TROUBLESHOOTING.md) for the Bash equivalent.

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

## Memory profiles

FindMyAdv has two intentional build profiles. The normal profile preserves all
NimBLE roles so it can share a stack with an application that scans, connects,
or exposes GATT services. A broadcaster-only application can remove those
unused roles globally:

```ini
build_flags =
    -DCONFIG_BT_NIMBLE_ROLE_CENTRAL_DISABLED
    -DCONFIG_BT_NIMBLE_ROLE_OBSERVER_DISABLED
    -DCONFIG_BT_NIMBLE_ROLE_PERIPHERAL_DISABLED
    -DCONFIG_BT_NIMBLE_MAX_CONNECTIONS=1
    -DCONFIG_BT_NIMBLE_LOG_LEVEL=5
    -DCONFIG_NIMBLE_CPP_LOG_LEVEL=0
```

Use these flags only if no other part of the firmware needs a NimBLE client,
scanner, or GATT server. They keep the broadcaster role required by
FindMyAdv. To produce the absolute-smallest build on hardware without a
LIS3DH/LIS2DH/LIS2DH12, add:

```ini
    -DFINDMYADV_DISABLE_BUILTIN_ACCELEROMETER=1
```

This last flag removes only the built-in I2C motion driver; a custom
`motionDetectedCallback` remains available.

Measured complete ESP32-C3 firmware sizes with Arduino-ESP32 2.0.17,
PlatformIO `espressif32@7.0.1`, valid disposable Apple/Google identities, and
manual polling are:

| Build | Flash | Static RAM |
| --- | ---: | ---: |
| Empty Arduino sketch | 247,502 B | 13,748 B |
| FindMyAdv 1.0.0 | 942,484 B | 38,972 B |
| 1.1 normal/coexistence profile | 481,268 B | 23,980 B |
| 1.1 broadcaster-only, accelerometer retained | 404,968 B | 21,252 B |
| 1.1 absolute-minimum, no built-in accelerometer | 382,178 B | 20,956 B |

These are total firmware sizes, not the size of `FindMyAdv.cpp` alone. If the
host application already uses NimBLE, the Bluetooth stack is shared and is
not linked a second time. `backgroundTask = false` additionally avoids the
optional scheduler task and its runtime stack; call `poll()` from the existing
loop in that mode.

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

## Generate and register advertisement keys

FindMyAdv is the BLE advertising component only. Key generation, tracker
registration, authentication, location-report retrieval, and report decryption
belong to the companion projects below. FindMyAdv does not contact Apple or
Google services and does not need their account credentials.

### Apple Find My / Haystack

Use [OpenHaystack](https://github.com/seemoo-lab/openhaystack) or a compatible
Haystack deployment such as
[macless-haystack](https://github.com/dchristl/macless-haystack) to create the
tracker key pair:

1. Generate a new accessory in OpenHaystack, then copy its **public
   advertisement key**; or run `generate_keys.py` from macless-haystack and
   read the Base64 advertisement key from the generated `.keys` file.
2. Set that public value as `appleAdvertisementKeyBase64`. It must decode to
   exactly 28 bytes (normally 40 Base64 characters including padding).
3. Keep the private key in the Haystack client/backend that decrypts the
   reports. Never put the private key, Apple credentials, or 2FA data in the
   firmware.

FindMyAdv only broadcasts the public advertisement key. Retrieving positions
still requires a correctly configured OpenHaystack/macless-haystack service.

### Google Find Hub / Google Find My Tools

Use [GoogleFindMyTools](https://github.com/leonboe1/GoogleFindMyTools) to
authenticate and register the custom tracker:

1. Follow its current setup instructions: install the Python requirements and
   run `main.py`.
2. Choose the custom-tracker registration command (`r` in the current tool),
   complete its prompts, and copy the displayed advertisement key/EID.
3. Set it as `googleAdvertisementEidHex`: exactly 40 hexadecimal characters
   (20 bytes), without `0x`, spaces, colons, or dashes.

Authentication secrets remain on the computer running GoogleFindMyTools; they
must not be copied to the ESP32. Registration and any periodic upstream
announcement required by GoogleFindMyTools also remain that tool's
responsibility. Its protocol and instructions are experimental and can change,
so check the upstream README when creating a tracker.

Configure either provider or both:

```cpp
FindMyAdvConfig tracking;
tracking.appleAdvertisementKeyBase64 = "APPLE_PUBLIC_KEY_BASE64"; // or ""
tracking.googleAdvertisementEidHex = "0123456789abcdef0123456789abcdef01234567"; // or ""

if (!FindMyAdv.begin(tracking)) {
    Serial.println(FindMyAdv.lastError());
}
```

Do not copy keys from commercial trackers or reuse one identity on unrelated
devices. Only register and track hardware that you own or are authorized to
manage.

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
#include <Wire.h>

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
Private keys, service passwords, browser authentication data, and 2FA secrets
must never be stored in the firmware. A fixed public identifier may also be
recognizable by nearby BLE observers, so evaluate that privacy trade-off for
the intended use.

## Examples

- `Minimal`: one-call integration.
- `DynamicKeys`: load keys from Preferences and replace them at runtime.
- `ManualPolling`: disable the FreeRTOS scheduler task and call `poll()` from
  an existing cooperative loop.
- `CustomMotionSource`: use any IMU, GPIO, or application event for dynamic
  advertising speed.

## License

MIT. See [`LICENSE`](LICENSE).
