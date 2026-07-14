# Changelog

All notable changes to FindMyAdv are documented here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the project uses
[Semantic Versioning](https://semver.org/).

## [1.1.0] - 2026-07-14

### Changed

- Replaced the default Bluedroid path with a single NimBLE-Arduino dependency
  on all tested ESP32 targets.
- Uses direct GAP advertising with fixed buffers instead of high-level BLE
  advertising containers.
- Added documented broadcaster-only flags that remove unused client, scanner,
  GATT-server, connection, and logging features.
- Reduced the optional scheduler task default from 4096 to 3072 bytes and made
  it configurable with `schedulerTaskStackBytes`.
- Made the built-in ST accelerometer driver compile-time removable without
  disabling custom motion callbacks.

### Fixed

- `begin()` now reports a first-advertisement failure instead of returning a
  false-positive running state on NimBLE.
- Rejects overlong and incorrectly padded advertisement keys instead of
  silently accepting a truncated value.
- Removed the unconditional `Wire.h` include from the public API header.
- Added CI builds for normal, broadcaster-only, absolute-minimum, and legacy
  Bluedroid configurations.

## [1.0.0] - 2026-07-14

### Added

- Apple Find My and Google Find Hub legacy advertising.
- Automatic Apple/Google provider rotation.
- ESP32/ESP32-C3/ESP32-S3 Bluedroid backend.
- ESP32-C6/ESP32-H2 NimBLE backend.
- Background FreeRTOS scheduler with optional manual polling.
- Runtime `setKeys()` and complete `reconfigure()` APIs.
- Optional LIS3DH/LIS2DH/LIS2DH12 dynamic advertising rate.
- Pluggable motion callback for any other sensor or application state.
- English and Italian documentation and three integration examples.
