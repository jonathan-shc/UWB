# openaliro on the ESP32-S3: flash guide

This bundle is the complete Aliro Matter lock on a single ESP32-S3: it
commissions into Apple Home over Wi-Fi, a key is provisioned into your iPhone's
Wallet, and the lock opens as you walk up (BLE + UWB approach unlock; there is
no NFC tap on this target).

In this bundle:

| File | What it is |
|---|---|
| `openaliro-matter-lock.bin` | one merged image (bootloader + partition table + app), flashed at offset 0x0 |
| `flash.sh` | flashes the image with esptool |
| `VERSION.txt` | release tag, commit, and build date |

## 1. What you need

Hardware:

| Part | Role |
|---|---|
| ESP32-S3 dev board | host: Wi-Fi, Matter, BLE, and the ranging engine. Validated on an ESP32-S3-WROOM N16R8 module (16 MB flash, 8 MB PSRAM); smaller variants are untested |
| Qorvo DWM3000EVB (DW3110) | UWB radio, hand-wired over SPI (11 connections, below) |
| 11 jumper wires | the SPI wiring |
| USB cable | powers the board, flashes it, and carries the serial console |

Apple side:

- An iPhone with a UWB chip. iOS 26 is the validated floor.
- An Apple Home hub (HomePod or Apple TV), and a 2.4 GHz Wi-Fi network for the
  lock to join.

On your computer (macOS, Linux, or Windows): Python 3, then `pip install esptool`,
and any serial terminal (`screen`, `minicom`, PuTTY).

## 2. Wire it

**First: check the DWM3000EVB's power-select jumper.** If it selects the wrong
source the radio fails silently: no valid device ID, a responder that never
listens, and it looks exactly like a software fault. This cost days on the
bench once. Check it against the EVB manual before anything else.

Power the EVB from the ESP32-S3 board's **3V3** pin, not 5 V (the DW3000 is a
3.3 V part), and share a common ground. The pin map below is transcribed from
the build's source of truth, `ports/esp32/components/woz_uwb/port/board_pins.h`
in the repository:

| DWM3000EVB (Arduino pin) | Signal | ESP32-S3 |
|---|---|---|
| D13 | SCLK | GPIO12 |
| D11 | MOSI | GPIO11 |
| D12 | MISO | GPIO13 |
| D10 | CS | GPIO10 |
| D8 | IRQ | GPIO5 |
| D7 | RSTn | GPIO4 |
| D9 | WAKEUP | GPIO6 |
| 3V3 | power | 3V3 |
| GND | ground | GND |
| D1 | SPI-POL | GND (mode-0 strap) |
| D0 | SPI-PHA | GND (mode-0 strap) |

The two straps hold the DW3000 in SPI mode 0; check the EVB manual before tying
them if your EVB revision already fixes the mode on the shield.

## 3. Flash

Plug the board in over USB, then:

```bash
bash flash.sh
```

esptool auto-detects the port; to name one, `bash flash.sh /dev/ttyACM0` (or
`/dev/cu.usbmodem…` on macOS, `COM…` on Windows). If the chip is not detected,
hold the board's BOOT button while plugging it in and retry.

Manual equivalent:

```bash
esptool.py --chip esp32s3 --baud 460800 write_flash 0x0 openaliro-matter-lock.bin
```

Reflashing wipes any previous commissioning; that is the expected clean-install
behavior.

## 4. First run

1. Open the board's serial port at 115200 baud. The boot log prints the Matter
   onboarding QR code URL and the manual pairing code; typing `codes` at the
   `matter>` prompt reprints them.
2. In the Home app, add an accessory with that QR code or manual code. The
   firmware carries Matter test certificates, so Home will warn about an
   uncertified accessory; that is expected for this evaluation build.
3. Once commissioned, Home provisions the key into Wallet on its own. Walk up
   with the phone: the Wallet animation plays and the lock opens, then relocks
   as you leave.

The `matter>` console also has `status`, `range`, `lock`/`unlock`, and
`factoryreset`.

## Notes

- This is evaluation firmware with test certificates. Do not put it in charge
  of a door you care about.
- Bring-up checklist and what good console output looks like:
  <https://github.com/asxeem/openaliro/blob/main/docs/esp32-bringup.md>
- Forty-odd known traps with symptoms and fixes:
  <https://github.com/asxeem/openaliro/blob/main/docs/esp32-gotchas.md>
