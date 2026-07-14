# Release and publication guide

## 1. Verify the source

Build every target from the standalone library repository:

```bash
pio run -d ci -e esp32
pio run -d ci -e esp32c3
pio run -d ci -e esp32s3
pio run -d ci -e esp32c6
```

Check that examples contain no real keys, update `CHANGELOG.md`, and set the
same semantic version in `library.json` and `library.properties`.

## 2. Validate the PlatformIO package

From the repository root:

```bash
pio pkg pack . --output FindMyAdv-1.0.0.tar.gz
```

Inspect the archive before publishing. The PlatformIO CLI validates the
manifest while packing it.

## 3. Publish GitHub source

Create the public `mattbox03/FindMyAdv` repository, push the verified source,
tag the same version, and attach the generated archive to the release. Do not
commit the personal `FindMyAdv-configured.zip` produced by Find_My_Web.

## 4. Publish to the PlatformIO Registry

Authenticate once and publish the verified source directory:

```bash
pio account login
pio pkg publish . --owner mattbox03
```

PlatformIO package name/version pairs are immutable after publication. Always
increment `version` before publishing changed source.

Verify the public package from a clean test project:

```ini
lib_deps = mattbox03/FindMyAdv@^1.0.0
```

Official references:

- <https://docs.platformio.org/en/latest/librarymanager/creating.html>
- <https://docs.platformio.org/en/latest/core/userguide/pkg/cmd_publish.html>
- <https://docs.platformio.org/en/latest/core/userguide/pkg/cmd_pack.html>

