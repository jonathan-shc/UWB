# DWM3000EVB → ESP32-S3 bring-up checklist

One page. When you're ready, this is match-the-table, not think-hard. Pin map is
the source of truth in `components/woz_uwb/port/board_pins.h` — if you change it
there, change it here.

## 1. Wire it (11 connections)

Power the EVB from the ESP32-S3 board's **3V3** pin (NOT 5V — the DW3000 is a
3.3 V part). Share a common ground. USB-power the board during bring-up.

| DWM3000EVB (Arduino pin) | signal   | → | ESP32-S3 |
|--------------------------|----------|---|----------|
| D13                      | SCLK     | → | GPIO12   |
| D11                      | MOSI     | → | GPIO11   |
| D12                      | MISO     | → | GPIO13   |
| D10                      | CS       | → | GPIO10   |
| D8                       | IRQ      | → | GPIO5    |
| D7                       | RSTn     | → | GPIO4    |
| D9                       | WAKEUP   | → | GPIO6    |
| 3V3                      | power    | → | 3V3      |
| GND                      | ground   | → | GND      |
| D1                       | SPI-POL  | → | GND (mode-0 strap) |
| D0                       | SPI-PHA  | → | GND (mode-0 strap) |

Mode-0 strap = the DW3000 SPI must run CPOL=0/CPHA=0. Tie D0 and D1 to GND unless
the EVB manual says the shield already fixes the mode (some revisions do — then
leave D0/D1 unconnected). Verify against the EVB manual before soldering.

Camera detached, so GPIO 4/5/6/10/11/12/13 are free. If any of those aren't
broken out on your board's headers, tell me — SPI2 routes through the S3 GPIO
matrix, so I remap to whatever pins you do have.

## 2. Build + flash

    . ~/esp/esp-idf/export.sh
    cd ports/esp32-idf
    idf.py set-target esp32s3      # once, if not already set
    idf.py build
    idf.py -p <PORT> flash monitor

Find `<PORT>`: `ls /dev/cu.usb*` (macOS). It's usually `/dev/cu.usbserial-*` or
`/dev/cu.usbmodem*`.

## 3. What good output looks like

The bring-up app binds a dummy URSK and starts the CCC DS-TWR responder, then
logs the start result and polls for a range:

    I (xxx) woz_esp32s3: woz_uwb_start_aliro() = 0 (DW3000 up, responder listening)

- `= 0` → SPI + DW3000 + CCC init path came up. The engine is talking to the
  chip. This is the Phase-1 pass criterion. No peer = no range lines, expected.
- `= <nonzero> (FAILED -- check wiring/SPI)` → the DW3000 didn't answer. First
  moves: recheck CS/SCLK/MOSI/MISO, confirm the mode-0 strap, then drop the clock
  to slow-only by setting `WOZ_DW3000_SPI_FAST_HZ` = `2000000` in `board_pins.h`.

## 4. If it comes up: prove a real range

Needs a peer driving the DS-TWR exchange — an Aliro-capable iPhone (Wallet key),
or a second DW3000 board as initiator. Then `range: NN cm` lines appear.
