# openaliro on the nRF5340 DK: flash guide

This bundle is the primary openaliro target: an Aliro lock with NFC tap and UWB
approach unlock, built on the Nordic door-lock add-on. It joins Apple Home as a
Matter-over-Thread accessory; your iPhone then carries the key in Wallet and the
lock opens as you walk up, or on an NFC tap.

In this bundle:

| File | What it is |
|---|---|
| `merged.hex` | application core image (Matter, Aliro, the UWB engine) |
| `merged_CPUNET.hex` | network core image (radio controller) |
| `flash.sh` | flashes both cores over the DK's on-board J-Link |
| `VERSION.txt` | release tag, commit, and build date |

## 1. What you need

Hardware:

| Part | Role |
|---|---|
| Nordic nRF5340 DK | host board: BLE, Thread, Matter, and the ranging engine |
| Qorvo DWM3000EVB (DW3110) | UWB radio; seats directly on the DK's Arduino header |
| ST X-NUCLEO-NFC12A1 (ST25R300) | NFC reader front end for tap; ribbon-wired (below) |
| 8 jumper wires or a ribbon | connects the NFC board to the DK |
| USB cable | powers the DK, flashes it, and carries the serial console |

Apple side:

- An iPhone with a UWB chip. iOS 26 is the validated floor.
- An Apple Home hub that can act as a Thread border router (HomePod or
  Apple TV 4K), since the lock joins the home over Thread.

On your computer (macOS, Linux, or Windows):

- [nRF Util](https://www.nordicsemi.com/Products/Development-tools/nRF-Util),
  then run `nrfutil install device` once.
- [SEGGER J-Link software](https://www.segger.com/downloads/jlink/) (the
  programming backend nrfutil uses).
- Any serial terminal (`screen`, `minicom`, PuTTY).

Prefer a GUI? The Programmer app in
[nRF Connect for Desktop](https://www.nordicsemi.com/Products/Development-tools/nRF-Connect-for-Desktop)
can flash both hex files instead of `flash.sh`.

## 2. Assemble

1. Seat the DWM3000EVB shield on the DK's Arduino header.
2. **Check the DWM3000EVB's power-select jumper.** If it selects the wrong
   source the radio fails silently: no valid device ID, a responder that never
   listens, and it looks exactly like a software fault. This cost days on the
   bench once. Check it against the EVB manual before anything else.
3. Wire the NFC12A1 to the DK. The pin map below is transcribed from the build's
   source of truth, `ports/nrf5340dk/overlays/dw3000-nfc.overlay` in the
   repository; connect the matching SPI signals on the NFC12A1 (see its silk
   screen or schematic) to these DK pins:

| Signal | nRF5340 DK pin |
|---|---|
| SPI SCK | P0.06 |
| SPI MOSI | P0.07 |
| SPI MISO | P0.25 |
| SPI CS | P0.26 |
| IRQ | P1.08 |
| RST | P1.07 |
| 3V3 | 3V3 |
| GND | GND |

The four SPI lines sit on the DK's A2 to A5 analog header positions.

## 3. Flash

Plug the DK in over USB (the J-Link port), then:

```bash
bash flash.sh
```

It programs the network core, then the application core, then resets the board.
Both cores are fully erased first, so any previous commissioning is wiped; this
is the expected clean-install behavior. To pick one DK among several, pass its
J-Link serial number: `bash flash.sh 1050012345`.

Manual equivalent:

```bash
nrfutil device program --firmware merged_CPUNET.hex --core network \
  --options chip_erase_mode=ERASE_ALL,verify=VERIFY_READ,reset=RESET_NONE
nrfutil device program --firmware merged.hex --core application \
  --options chip_erase_mode=ERASE_ALL,verify=VERIFY_READ,reset=RESET_SYSTEM
```

## 4. First run

1. Open the DK's serial port at 115200 baud (the DK enumerates several ports;
   the console is usually the first). The boot log prints the Matter onboarding
   QR code URL and the manual pairing code. Press the DK's RESET button to see
   them again.
2. In the Home app, add an accessory with that QR code or manual code. The
   firmware carries Matter test certificates, so Home will warn about an
   uncertified accessory; that is expected for this evaluation build.
3. Once commissioned, Home provisions the key into Wallet on its own. Then walk
   up with the phone (unlock on approach) or tap it on the NFC antenna.

## Notes

- This is evaluation firmware with test certificates. Do not put it in charge
  of a door you care about.
- Troubleshooting, wiring checks, and a radio self-test:
  <https://github.com/asxeem/openaliro/blob/main/docs/troubleshooting.md>
