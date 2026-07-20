# `woz_uwb` — the shared engine, on ESP-IDF

This component contains almost no code of its own. Its job is to let
`modules/woz_uwb/src` and `deps/dw3000` — written for Zephyr, and shipped on the nRF5340
build — compile for the ESP32-S3 **unchanged**. Everything here is the two seams that
make that possible.

Not editing the shared sources is the point. They are the layer the nRF has already
proven correct; forking them for ESP32 would mean two engines to keep honest.

## `compat/zephyr/` — the API seam

A small fake `<zephyr/*>` tree that resolves first on the include path:

| Header | Backed by |
|---|---|
| `kernel.h` | FreeRTOS, `esp_timer`, `esp_rom_delay_us`, `esp_cpu_get_cycle_count` |
| `logging/log.h` | `esp_log`, mapping each module's log macros to a per-file tag |
| `sys/byteorder.h`, `sys/util.h`, `sys/printk.h` | plain C |

It is the on-silicon twin of the host-test shim in `tests/host/shim/`. Where they differ,
one of them is wrong.

## `port/` — the hardware seam

The ESP-IDF DW3000 backend, replacing `deps/dw3000/platform/`:

- **`dw3000_spi.c`** — SPI master with chip select driven as a plain GPIO, held low across
  a whole DW3000 command. **DMA is disabled on purpose.** A boot benchmark measured about
  84 µs per transaction with DMA, nearly all of it descriptor setup and cache sync, which
  dwarfs the bit time for the small register writes on the arming critical path. Clock
  speed is not the lever: 2 MHz versus 8 MHz was about 75 versus 84 µs. Transfers larger
  than the hardware buffer are chunked under one chip-select window, which the DW3000
  streams through sequentially.
- **`dw3000_hw.c`** — GPIO reset, wakeup, and the interrupt path. The ISR only gives a
  semaphore; a dedicated high-priority task pinned to core 1 calls `dwt_isr()`, because
  that call performs SPI and cannot run in interrupt context.
- **`board_pins.h`** — the pin map. Source of truth; the wiring table in
  [`../../BRINGUP.md`](../../BRINGUP.md) mirrors it.
- **`woz_wrap_stubs.c`** — the minimal RX-callback chain that the excluded diagnostic
  sources would otherwise have provided. Without it the responder receives frames but
  never replies.

## Configuration and link seam

Engine layers are selected as compile definitions rather than Kconfig, mirroring what the
in-tree `CMakeLists.txt` gates on. Diagnostic and Nordic-specific sources are excluded.

The STS substitution links exactly as it does on Nordic, through `--wrap`. The
load-bearing one is `dwt_rxenable`, where the CCC key and IV are programmed on every
RX-arm. `../../test/verify_port.sh` checks after each build that the wrap flags are still
in the link, that the wrapper symbols are defined, that the engine still references the
wrapped symbol, and that the excluded sources stayed out.

Warnings are relaxed for this component only (`-Wno-format`,
`-Wno-maybe-uninitialized`), because ESP-IDF builds with `-Werror` and the shared sources
were written where `int32_t` is `int`. This changes no behavior, and it applies to no
other component.
