#!/usr/bin/env bash
#
# flash.sh — program the openaliro ESP32-S3 Matter lock (single merged image at
# offset 0x0) with esptool. See FLASH.md for wiring and first run.
#
# Usage:  bash flash.sh [PORT]       e.g.  bash flash.sh /dev/ttyACM0
set -euo pipefail
cd "$(dirname "$0")"

if command -v esptool.py >/dev/null 2>&1; then
  ESPTOOL=(esptool.py)
elif command -v esptool >/dev/null 2>&1; then
  ESPTOOL=(esptool)
elif python3 -m esptool version >/dev/null 2>&1; then
  ESPTOOL=(python3 -m esptool)
else
  echo "ERROR: esptool not found. Install it with:  pip install esptool"
  exit 1
fi
[ -f openaliro-matter-lock.bin ] || {
  echo "ERROR: openaliro-matter-lock.bin not found next to this script."
  exit 1
}

PORT=()
[ $# -ge 1 ] && PORT=(--port "$1")

"${ESPTOOL[@]}" --chip esp32s3 ${PORT[@]+"${PORT[@]}"} --baud 460800 \
  write_flash 0x0 openaliro-matter-lock.bin

echo "==> done. Open the serial port at 115200 baud for the commissioning QR code."
