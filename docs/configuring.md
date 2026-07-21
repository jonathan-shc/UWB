# Configuring

Three layers. Build options on the make command line. Kconfig overlays behind
them. Runtime consoles on a running reader.

## Build options (nRF5340)

Set on the command line, e.g. `make build PRETTY=1 CHIP=dw3720`:

| Option | Effect |
|---|---|
| `CHIP=dw3720` | build for the DW3720 (default: DW3000) |
| `PRETTY=1` | curated, quiet serial console |
| `SELFTEST=1` | radio TX/RX self-test at boot, no iPhone needed |
| `STRICT=1` | drop suspect UWB range blocks |
| `HA=1` | Home Assistant variant; needs `make bootstrap HA=1` too |
| `PRISTINE=1` | force a clean rebuild |

## Kconfig overlays

They live in [`../ports/nrf5340dk/overlays`](../ports/nrf5340dk/overlays) and
layer over the stock Nordic door-lock app. The upstream tree stays pristine.
Each file documents every setting it touches.

* `woz-aliro.conf`: always applied. UWB heap and threads, BLE time-sync,
  the Apple ECP Express tap, log levels.
* `woz-pretty.conf`, `woz-ha.conf`: opt-in via `PRETTY=1` / `HA=1`.
* `diag-latency.conf`: diagnostic only (`LAT=1` to `scripts/build.sh`).
  Matter debug logs, for timing notification delays.

## ESP32-S3

One project option in `idf.py menuconfig`: **Enable Aliro over BLE + UWB**,
default on. It advertises the Aliro features so Apple Home can put a key in
Wallet. Commissioning is standard Matter over Wi-Fi. The `codes` console
command reprints the QR URL and pairing code.

## Runtime consoles

Every firmware has a serial console. No reflash needed.

**nRF5340** (`make term`): the `aliro` command group — `status`, `rx`,
`range`, `chip`, `selftest`, `log`, `frames`, `version`.

**ESP32 Matter lock** (`make monitor`): `status`, `lock`, `unlock`, `codes`,
`range`, `factoryreset`, `aliro <prov|trust|clear>`.

**ESP32 reader** (`make monitor`): `status`, `range`, `aliro-start` /
`aliro-stop` (demo responder, no phone needed), `aliro-prov`, `aliro-trust`.

`aliro trust` / `aliro-trust` persist the last-seen credential to NVS. It
survives reboots. `factoryreset` and `flash-erase` drop it.

## Where the defaults are

Reader identity and the trust store are NVS-backed, created on first boot.
Use the consoles to inspect or reset them. Nothing to hand-edit on disk.
