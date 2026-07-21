# Configuring

What can be tuned, where it lives, and which knobs are safe to turn: build
options on the make command line, the Kconfig overlays behind them, and the
runtime consoles for inspecting and provisioning a running reader.

## Build options (nRF5340)

All options go on the make command line, e.g. `make build PRETTY=1 CHIP=dw3720`:

| Option | Effect |
|---|---|
| `CHIP=dw3720` | build for the DW3720 instead of the default DW3000 |
| `PRETTY=1` | curated, quiet serial console (layers `woz-pretty.conf`) |
| `SELFTEST=1` | run the radio TX/RX self-test at boot, no iPhone needed |
| `STRICT=1` | drop suspect UWB range blocks (`CONFIG_WOZ_RANGE_GATE_STRICT`) |
| `HA=1` | Home Assistant variant; needs `make bootstrap HA=1` too |
| `PRISTINE=1` | force a clean reconfigure and rebuild |

## Kconfig overlays

The overlays live in [`../ports/nrf5340dk/overlays`](../ports/nrf5340dk/overlays)
and layer on top of the stock Nordic door-lock app, so the upstream tree stays
pristine. Each file documents every setting it touches:

* `woz-aliro.conf` is always applied. It holds the settings that make the
  UWB engine work at all (heap for the UWB stack, dynamic threads, SPI
  power management), the Aliro BLE time-sync advertisement, the Apple ECP
  Express tap, and the log levels.
* `woz-pretty.conf`, `woz-ha.conf` layer on demand via `PRETTY=1` / `HA=1`.
* `diag-latency.conf` is a diagnostic (`LAT=1` to `scripts/build.sh`): Matter
  at debug log level to timestamp where a notification delay sits.

## ESP32-S3

The Matter lock app exposes one project option in `idf.py menuconfig`
(Example Configuration): **Enable Aliro over BLE + UWB**, default on, which
advertises the Door Lock cluster's Aliro features so Apple Home can provision
an Express key into Wallet. Commissioning itself is standard Matter over
Wi-Fi; the `codes` console command reprints the QR URL and pairing code.

## Runtime consoles

Each firmware has a serial console for inspecting and provisioning the
running reader; no reflash needed.

**nRF5340** (`make term`, Zephyr shell): the `aliro` command group, with
`status`, `rx`, `range`, `chip`, `selftest`, `log`, `frames` and `version`
subcommands.

**ESP32 Matter lock** (`make monitor`): `status`, `lock`, `unlock`, `codes`,
`range`, `factoryreset`, and `aliro <prov|trust|clear>` for the credential
trust store.

**ESP32 reader** (`make monitor`): `status`, `range`, `aliro-start` /
`aliro-stop` (demo DS-TWR responder, canned key, no phone needed),
`aliro-prov`, and `aliro-trust`.

`aliro trust` / `aliro-trust` persist the last-presented credential to NVS,
so it survives reboots; `factoryreset` and `flash-erase` both drop it.

## Where the defaults are

Reader identity and the credential trust store are NVS-backed and created on
first boot. The consoles above are the supported way to inspect or reset
them; there is nothing to hand-edit on disk.
