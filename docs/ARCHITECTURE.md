# Architecture

FindMyAdv separates the host application from provider frame construction,
BLE backend selection, provider rotation, and optional motion sensing.

## Lifecycle

1. The host creates `FindMyAdvConfig` from constants, Preferences/NVS, a web
   form, MQTT, or another settings source.
2. `begin()` copies both key strings and all scalar settings. Caller-owned
   strings may be released immediately after the call returns.
3. The Apple Base64 key and Google hexadecimal EID are decoded and their raw
   legacy advertising frames are built.
4. The Arduino core's compile-time configuration selects Bluedroid or NimBLE.
   Selection is stack-based rather than a hard-coded chip list, allowing new
   BLE-capable ESP32 variants to use the library without source changes. An already initialized
   controller is reused; FindMyAdv never deinitializes a shared controller.
5. The scheduler publishes one provider. When both are enabled it rotates them
   after `providerWindowMs`.
6. By default a small FreeRTOS task polls motion and the provider scheduler.
   Manual mode delegates this step to the host's `loop()`.
7. `reconfigure()` stops the scheduler, copies a complete new configuration,
   and restarts it. `setKeys()` is a convenience wrapper preserving all other
   settings.

## Backends

The Bluedroid backend uses the raw advertising-data completion event before
starting the advertiser. Configuration is asynchronous in ESP-IDF; starting
before that event is a common cause of apparently successful firmware that
never transmits.

The NimBLE backend runs the host task supplied by the Arduino/ESP-IDF port and
uses the synchronous GAP advertising-data API. Address byte order is converted
for NimBLE's least-significant-byte-first representation.

## BLE ownership

Legacy ESP32 controllers expose a limited advertising resource. FindMyAdv owns
that advertiser while running. This is independent from the host application's
main loop, but not independent from another component attempting to configure
the same BLE advertising set. A firmware that already advertises a connectable
GATT service must implement a shared schedule or pause one advertiser before
starting the other.

## Memory and timing

All key and frame storage is fixed-size. No heap allocation is performed by
the library itself after startup, aside from resources allocated internally by
the selected ESP Bluetooth stack and FreeRTOS task creation. The scheduler
task uses a 4096-byte stack and priority 1.
