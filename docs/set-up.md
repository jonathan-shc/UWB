# Installing

Everything to install, per target. The nRF5340 DK is the primary, validated
target; the ESP32-S3 apps are ports that share the same engine. Nothing here
needs hardware until the flash step, and the host test suite needs no
toolchain at all.

## nRF5340 DK (primary target)

Two commands, once per machine and once per checkout:

```bash
nrfutil sdk-manager toolchain install --ncs-version v3.3.0   # once per machine
make bootstrap   # NCS v3.3.0 + the Nordic add-on (~6.5 GB) into ./workspace
```

`make bootstrap` fetches the nRF Connect SDK and Nordic's door-lock add-on at
the pinned revision, then applies this repository's integration patches on
top. The workspace is git-ignored and lives in `./workspace` by default; the
knobs it honors:

| Knob | Effect |
|---|---|
| `ALIRO_WS=/big/disk/ws` | put the workspace somewhere else |
| `ALIRO_SHALLOW=1` | shallow fetch, saves several GB (what CI uses) |
| `ALIRO_TOOLCHAIN=env` | use the toolchain already on `PATH` instead of the nrfutil launcher |
| `HA=1` | also apply the Home Assistant data-model patches (pair with `make build HA=1`) |

Then build and flash:

```bash
make build         # merged image lands in ./build/merged.hex
make flash-erase   # first flash of a net-core image; plain `make flash` after
make term          # serial console on the DK's VCOM1
```

The full set of build options is in [configuring.md](configuring.md).

## Host tests (no toolchain)

The host suite compiles with a plain C compiler and finishes in about a
second, so it works before any of the above:

```bash
make test       # host KAT suite
make coverage   # the same suite under gcov, with the CI floor
```

## ESP32-S3 ports

Both apps expect ESP-IDF at `~/esp/esp-idf`; override with
`IDF_EXPORT=/path/to/esp-idf/export.sh` on any make target. CI builds against
ESP-IDF v5.5.4, and esp-matter at the revision pinned in
[`firmware-builds.yml`](../.github/workflows/firmware-builds.yml).

**Reader** (`../ports/esp32/apps/reader`) is plain ESP-IDF, no esp-matter:

```bash
cd ports/esp32/apps/reader
make set-target   # once per checkout (esp32s3)
make build && make flash && make monitor
```

**Matter door lock** (`../ports/esp32/apps/matter-lock`) additionally needs
esp-matter, expected at `~/esp/esp-matter` (override with `ESP_MATTER_PATH=`):

```bash
cd ports/esp32/apps/matter-lock
make set-target   # once per checkout
make go           # build + flash + monitor, the usual bench loop
```

On both apps, `flash` and `monitor` auto-select the board's USB-UART and
refuse any SEGGER/J-Link port, so they can never write to an nRF5340 DK
sharing the bench.

Wiring for the DWM3000EVB is in [esp32-bringup.md](esp32-bringup.md), and the
traps actually hit during the port are in [esp32-gotchas.md](esp32-gotchas.md).

## Documentation site

```bash
brew install doxygen graphviz
make docs   # builds this site into ./site
```

## If something fails

Common install and first-flash failures, with symptoms, are collected in
[troubleshooting.md](troubleshooting.md).
