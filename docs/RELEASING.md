# Release and publication guide

## 1. Verify the source

Build every target from the standalone library repository:

```bash
pio run -d ci -e esp32
pio run -d ci -e esp32c3
pio run -d ci -e esp32c3_broadcaster
pio run -d ci -e esp32c3_minimal
pio run -d ci -e esp32c3_bluedroid
pio run -d ci -e esp32s3
pio run -d ci -e esp32c6
```

Check that examples contain no real keys, update `CHANGELOG.md`, and set the
same semantic version in `library.json` and `library.properties`.

## 2. Validate the PlatformIO package

From the repository root:

```bash
pio pkg pack . --output FindMyAdv-1.1.0.tar.gz
```

Inspect the archive before publishing. The PlatformIO CLI validates the
manifest while packing it.

## 3. Publish GitHub source

Use the public `mattbox03/Find_My_adv_ESP_library` repository, push the verified source,
tag the same version, and attach the generated archive to the release. Do not
commit the personal `FindMyAdv-configured.zip` produced by Find_My_Web.

## 4. Verify installation from GitHub

From a clean PlatformIO project, reference the public repository:

```ini
lib_deps = https://github.com/mattbox03/Find_My_adv_ESP_library.git
```

Build that project to verify the tagged GitHub source can be installed without
a local library copy. A PlatformIO Registry account is not required.

Official references:

- <https://docs.platformio.org/en/latest/librarymanager/creating.html>
- <https://docs.platformio.org/en/latest/core/userguide/pkg/cmd_pack.html>
