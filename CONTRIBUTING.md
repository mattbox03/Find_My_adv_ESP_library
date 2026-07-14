# Contributing

Contributions are welcome through GitHub issues and pull requests.

## Before opening an issue

- Remove all real Apple advertisement keys, Google EIDs, account details, and
  private network addresses from logs and examples.
- State the exact ESP chip, board, Arduino core, PlatformIO platform, and
  whether another component initializes or advertises BLE.
- Include the return value of `begin()`, `status()`, and `lastError()`.

## Development checks

Build the standalone integration project for every maintained environment:

```bash
pio run -d ci -e esp32
pio run -d ci -e esp32c3
pio run -d ci -e esp32c3_broadcaster
pio run -d ci -e esp32c3_minimal
pio run -d ci -e esp32c3_bluedroid
pio run -d ci -e esp32s3
pio run -d ci -e esp32c6
```

New public API must be documented in `docs/API.md`, and behavior changes must
be added to `CHANGELOG.md`. Examples must use empty placeholder keys.

## Compatibility policy

Patch releases fix bugs without changing the public API. Minor releases add
backward-compatible options. Breaking API or configuration changes require a
new major version.
