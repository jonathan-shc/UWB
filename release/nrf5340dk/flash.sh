#!/usr/bin/env bash
#
# flash.sh — program the openaliro nRF5340 DK firmware (both cores) over the
# DK's on-board J-Link, using nrfutil. See FLASH.md for setup and first run.
#
# Usage:  bash flash.sh [JLINK_SERIAL_NUMBER]
set -euo pipefail
cd "$(dirname "$0")"

command -v nrfutil >/dev/null 2>&1 || {
  echo "ERROR: nrfutil not found."
  echo "  Install it from https://www.nordicsemi.com/Products/Development-tools/nRF-Util"
  echo "  then run:  nrfutil install device"
  exit 1
}
nrfutil device --help >/dev/null 2>&1 || {
  echo "ERROR: the nrfutil 'device' plugin is missing. Run:  nrfutil install device"
  exit 1
}
[ -f merged.hex ] && [ -f merged_CPUNET.hex ] || {
  echo "ERROR: merged.hex / merged_CPUNET.hex not found next to this script."
  exit 1
}

SNR=()
[ $# -ge 1 ] && SNR=(--serial-number "$1")

echo "==> network core (merged_CPUNET.hex)"
nrfutil device program --firmware merged_CPUNET.hex --core network \
  --options chip_erase_mode=ERASE_ALL,verify=VERIFY_READ,reset=RESET_NONE \
  ${SNR[@]+"${SNR[@]}"}

echo "==> application core (merged.hex)"
nrfutil device program --firmware merged.hex --core application \
  --options chip_erase_mode=ERASE_ALL,verify=VERIFY_READ,reset=RESET_SYSTEM \
  ${SNR[@]+"${SNR[@]}"}

echo "==> done. Open the DK's serial port at 115200 baud for the commissioning QR code."
