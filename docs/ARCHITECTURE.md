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
4. NimBLE-Arduino initializes or reuses the BLE host. FindMyAdv never tears
   down a shared controller. Bluedroid is an explicit compatibility option,
   not the default.
5. The scheduler publishes one provider. When both are enabled it rotates them
   after `providerWindowMs`.
6. By default a small FreeRTOS task polls motion and the provider scheduler.
   Manual mode delegates this step to the host's `loop()`.
7. `reconfigure()` stops the scheduler, copies a complete new configuration,
   and restarts it. `setKeys()` is a convenience wrapper preserving all other
   settings.

## Backends

The default NimBLE backend uses `NimBLEDevice` only for portable stack
initialization and controller power/address setup. Advertising itself uses the
synchronous raw GAP API, fixed 29/31-byte buffers, and no C++ advertising-data
containers. Address byte order is converted for NimBLE's
least-significant-byte-first representation.

The optional Bluedroid backend uses the raw advertising-data completion event
before starting the advertiser. Its configuration is asynchronous in ESP-IDF;
starting before that event is a common cause of apparently successful firmware
that never transmits.

## BLE ownership

Legacy ESP32 controllers expose a limited advertising resource. FindMyAdv owns
that advertiser while running. This is independent from the host application's
main loop, but not independent from another component attempting to configure
the same BLE advertising set. A firmware that already advertises a connectable
GATT service must implement a shared schedule or pause one advertiser before
starting the other.

## Memory and timing

All key and frame storage is fixed-size. Provider changes call GAP directly and
perform no advertising-payload heap allocation. The optional scheduler task
uses 3072 bytes by default and priority 1; manual polling creates no task.
NimBLE remains the only external runtime dependency. The broadcaster-only
profile compiles out unrelated BLE roles and logging, while the normal profile
keeps them for coexistence with a larger host application.
