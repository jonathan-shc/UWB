# Woz UWB engine on ESP32-S3 (ESP-IDF port)

Phase 1 of the ESP32 port (see `docs/porting-esp32.md`): the openaliro UWB ranging
engine + the DW3000 decadriver, building and linking on ESP-IDF for the ESP32-S3,
with a canned-URSK CCC DS-TWR responder bring-up app.

Status: **builds clean for `esp32s3`** (`idf.py build` produces `woz_uwb_esp32s3.bin`).
Not yet run on silicon: a live range needs a DWM3000EVB wired up (still to source)
and a peer that drives the DS-TWR exchange (an Aliro Wallet, or a second board).

## What this is (and how it stays additive)

This tree reuses `modules/woz_uwb/src` and `deps/dw3000` from the repo unchanged.
Only one seam is target-specific now:

- `components/woz_uwb/port/` — the ESP-IDF DW3000 platform backend
  (`dw3000_spi.c`, `dw3000_hw.c`) replacing the Zephyr `deps/dw3000/platform/`
  `dw3000_spi.c`/`dw3000_hw.c` (SPI-master + GPIO/IRQ), plus `board_pins.h` and a
  tiny wrap/diag stub (`woz_wrap_stubs.c`).

There is **no Zephyr compatibility layer**. The engine takes its whole OS surface
from `modules/woz_uwb/src/facade/woz_port.h` (eight functions: heap, monotonic
clock, two sleeps, cycle counter) and its logging from `woz_log.h`, both of which
select an ESP-IDF backend on `ESP_PLATFORM`. The earlier `compat/zephyr/*` shim,
194 lines of fake `<zephyr/*>` headers, was deleted once those two headers
existed. See [`docs/porting.md`](../../docs/porting.md).

The CCC STS substitution seam (`-Wl,--wrap=dwt_configurestsiv/dwt_rxenable/
dwt_configurestsmode`) links and is wired on `xtensa-esp32s3-elf-ld`.

## Source set

Mirrors `modules/woz_uwb/CMakeLists.txt` + `deps/dw3000/CMakeLists.txt` for
`CONFIG_WOZ_UWB` + `RESPONDER` + `ALIRO` + `CRYPTO_MBEDTLS`. Excluded (all
diagnostic/host-specific): `uwb_rxdiag.c`, `uwb_selftest.c`, `woz_logfmt.c`,
`woz_logquiet.c`, `aliro_shell.c`, `dw3000_spi_trace.c`, `ccc_crypto_psa.c`
(Nordic PSA backend — mbedTLS is used here).

## Wiring (DWM3000EVB -> ESP32-S3, edit in `components/woz_uwb/port/board_pins.h`)

    SPI2/FSPI:  SCLK 12   MOSI 11   MISO 13   CS 10
    control:    RSTn 4    IRQ 5     WAKEUP 6
    clock:      2 MHz init / 8 MHz steady

## Build / flash

    . ~/esp/esp-idf/export.sh
    cd ports/esp32-idf
    idf.py set-target esp32s3      # once
    idf.py build
    idf.py -p <PORT> flash monitor # needs the board + DWM3000EVB

## Known hardware-bring-up risks (verify when the DWM3000EVB arrives)

- SPI transfers use a DMA-capable bounce buffer + manual GPIO CS held low across
  each DW3000 command. Confirm the DW3000 tolerates the inter-segment gaps and the
  8 MHz clock on your wiring; drop to `WOZ_DW3000_SPI_SLOW_HZ` first if reads are
  garbage.
- `-Wno-format`/`-Wno-maybe-uninitialized` are set for the reused component only
  (ESP-IDF `-Werror=all` vs the Nordic toolchain where `int32_t == int`). They do
  not affect behavior.
- The IRQ runs a dedicated core-1 task calling `dwt_isr()` while the line is high
  (mirrors the Nordic dedicated-workqueue design); tune its priority against BLE
  when coexistence is added.
