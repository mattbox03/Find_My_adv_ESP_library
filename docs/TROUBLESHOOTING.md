# Troubleshooting

## `begin()` returns false

Read `FindMyAdv.lastError()` and `FindMyAdv.status()`.

- `NoValidKey`: Apple must decode to exactly 28 bytes from its 40-character
  Base64 representation; Google must be exactly 40 hexadecimal characters.
- `BleInitializationFailed`: another library may own an incompatible BLE host,
  or the selected Arduino core may not support that chip's Bluetooth stack.
- `TaskCreationFailed`: the application does not have enough free RAM for the
  4096-byte scheduler task. Use manual polling when necessary.

## The firmware runs but no packet is visible

- Verify at least one provider with `isAppleEnabled()` or `isGoogleEnabled()`.
- Scan with nRF Connect and inspect raw advertising data instead of filtering
  by a human-readable device name; tracker frames do not advertise one.
- Keep the scanner near the ESP and temporarily select 0 dBm or more.
- Ensure no other part of the firmware calls a BLE advertising API after
  FindMyAdv starts.
- Do not enter deep sleep while expecting continuous advertising.

## Only Apple or Google appears

With two valid keys, wait longer than `providerWindowMs` while scanning. The
providers share one legacy advertising set and are intentionally time-sliced.
Check `isAppleEnabled()` and `isGoogleEnabled()` to detect a malformed key.

## Motion mode never activates

- Confirm `isAccelerometerDetected()` returns true.
- Check SDA/SCL, 3.3 V, ground, and addresses `0x18`/`0x19`.
- If the host initialized the bus, pass its `TwoWire` instance and set
  `initializeAccelerometerBus = false`.
- Lower `motionThreshold` for greater sensitivity.

## The host's BLE service stopped advertising

FindMyAdv and the service are trying to own the same legacy advertising set.
Stop one before starting the other or implement an application-level schedule.
This cannot be solved transparently by two independent libraries.

## C6/H2 does not compile

Use an Arduino-ESP32 3.x platform that includes NimBLE for the selected target.
The repository's `ci/platformio.ini` pins a tested C6-compatible pioarduino
platform and Arduino-ESP32 core.

PlatformIO stores packages by canonical name. If Arduino-ESP32 2.x is already
installed, it may be selected instead of a same-named 3.x override and cause an
early framework error such as `FRAMEWORK_DIR None`. Isolate the C6 packages
from the global cache:

PowerShell:

```powershell
$env:PLATFORMIO_PACKAGES_DIR="$PWD\.pio-packages-c6"
pio run -e esp32c6
```

Bash:

```bash
PLATFORMIO_PACKAGES_DIR="$PWD/.pio-packages-c6" pio run -e esp32c6
```

Keep the `platform` and `platform_packages` values shown in the main README.
The first build downloads the complete framework and RISC-V toolchain; later
builds reuse the isolated directory.
