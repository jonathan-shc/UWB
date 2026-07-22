# ESP32 bring-up (S3 and C5)

One page, match-the-table. The pin map's source of truth is
`ports/esp32/components/woz_uwb/port/board_pins.h`; if you change it there, change it
here.

## 1. Wire it (11 connections)

Power the EVB from the ESP32 board's **3V3** pin, not 5 V — the DW3000 is a 3.3 V
part. Share a common ground and USB-power the board during bring-up.

| DWM3000EVB (Arduino pin) | Signal | → | ESP32-S3 | → | ESP32-C5 |
|---|---|---|---|---|---|
| D13 | SCLK | → | GPIO12 | → | GPIO8 |
| D11 | MOSI | → | GPIO11 | → | GPIO9 |
| D12 | MISO | → | GPIO13 | → | GPIO23 |
| D10 | CS | → | GPIO10 | → | GPIO10 |
| D8 | IRQ | → | GPIO5 | → | GPIO5 |
| D7 | RSTn | → | GPIO4 | → | GPIO4 |
| D9 | WAKEUP | → | GPIO6 | → | GPIO6 |
| 3V3 | power | → | 3V3 | → | 3V3 |
| GND | ground | → | GND | → | GND |
| D1 | SPI-POL | → | GND (mode-0 strap) | → | same |
| D0 | SPI-PHA | → | GND (mode-0 strap) | → | same |

Mode-0 strap: the DW3000 SPI must run CPOL=0/CPHA=0. Tie D0 and D1 to GND unless your
EVB revision already fixes the mode on the shield — check the EVB manual before
soldering.

GPIO 4, 5, 6, and 10-13 are clear of the octal PSRAM pins. SPI2 routes through the
GPIO matrix on both chips, so any of these can be remapped in `board_pins.h` if your
board does not break them out.

Why the C5 data pins differ: on the C5 the S3's GPIO11/12 are the UART0 console and
GPIO13 is USB-Serial-JTAG; GPIO8/9/23 also avoid the strapping pins (2/7/25/27/28,
plus 3/26 per the DevKitC-1 guide) and the GPIO27 RGB LED. The C5 build targets the
4 MB flash floor of the WROOM-1 family (`partitions_4mb.csv`); if your module has
more, raise `CONFIG_ESPTOOLPY_FLASHSIZE` in `sdkconfig.defaults.esp32c5`.

### Check the EVB power-select jumper

Do this before anything else. Correct wiring is not enough if the EVB's own power-select
jumper picks the wrong source: SPI then fails silently, with no valid device ID and a
responder that never listens, and it looks exactly like a software fault. This cost days
of debugging once. Check the jumper first.

## 2. Build and flash

```bash
cd ports/esp32/apps/reader
idf.py set-target esp32s3    # or: idf.py set-target esp32c5
make build
make flash
make monitor
```

`idf.py set-target` runs once per checkout. ESP-IDF is expected at
`~/esp/esp-idf`; override with `IDF_EXPORT=`. The port is
auto-detected and SEGGER/J-Link ports are refused; `make ports` lists what is attached
and how each is classified.

## 3. What good output looks like

The bench app brings the radio up, binds a canned URSK, and starts the CCC DS-TWR
responder:

    I (xxx) woz_esp32s3: woz_uwb_start_aliro() = 0 (DW3000 up, responder listening)

- `= 0` — SPI, DW3000, and the CCC init path all came up. The engine is talking to the
  chip. With no peer present there are no range lines, which is expected.
- `= <nonzero> (FAILED -- check wiring/SPI)` — the DW3000 did not answer. In order:
  recheck the power-select jumper, then CS/SCLK/MOSI/MISO, then the mode-0 strap, then
  drop to slow-only by setting `WOZ_DW3000_SPI_FAST_HZ` to `2000000` in `board_pins.h`.

## 4. Prove a real range

Ranging needs a peer to drive the DS-TWR exchange: an Aliro-capable iPhone with a key
provisioned for this reader, or a second DW3000 board acting as initiator. With a peer,
`range: NN cm` lines appear and `status` reports a trusted range.

For the full approach-unlock path — commissioning, a key in the phone's wallet, and the
Wallet unlock animation — use the Matter app in
[`ports/esp32/apps/matter-lock`](../ports/esp32/apps/matter-lock) instead. This bench
app has no Matter layer, so nothing provisions a real credential into a phone for it.

No antenna calibration was needed on this hardware. If distances come out negative or
absurd, read
[`docs/esp32-gotchas.md`](esp32-gotchas.md) §6.4 before reaching for a
calibration constant — it was a timestamp-pairing bug, not a physical offset.
