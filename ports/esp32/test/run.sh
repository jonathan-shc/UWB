#!/usr/bin/env bash
#
# Test entry point for the ESP32 port. Three layers, all hardware-free:
#   - test_port_headers: fast host unit test of the pure port headers.
#   - test_aliro_crypto: host KAT of the Aliro key-schedule core (SHA-256/KDF),
#                        compiled from the same source as the target.
#   - verify_port.sh:    on-target build + --wrap seam + exclusion guard (needs
#                        the ESP-IDF env; skips cleanly without it).
#
# On-target functional tests (Unity on the DW3000 SPI/IRQ path) are deferred:
# they need the DWM3000EVB wired up. See ../../../docs/esp32-bringup.md.
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"

echo "== host: port headers unit test =="
BIN="$(mktemp -t woz_port_headers.XXXXXX)"
trap 'rm -f "$BIN" "${CBIN:-}"' EXIT
cc -std=c11 -O1 -Wall -Wextra \
   -I "$HERE/../../../modules/woz_port/include" \
   -I "$HERE/../../../modules/woz_uwb/src/facade" \
   "$HERE/test_port_headers.c" -o "$BIN"
"$BIN"

echo
echo "== host: aliro_crypto key-schedule KAT =="
# The Aliro core is shared with the nRF build; it lives in modules/woz_aliro.
ALIRO="$HERE/../../../modules/woz_aliro"
CBIN="$(mktemp -t aliro_crypto_kat.XXXXXX)"
cc -std=c11 -O1 -Wall -Wextra \
   -I "$ALIRO/include" -I "$ALIRO/src" \
   "$HERE/test_aliro_crypto.c" \
   "$ALIRO/src/aliro_hash.c" "$ALIRO/src/aliro_crypto.c" "$ALIRO/src/aliro_advtag.c" \
   "$HERE/aliro_prim_host.c" -o "$CBIN"
"$CBIN"

echo
echo "== host: aliro_apdu wire-codec KAT =="
ABIN="$(mktemp -t aliro_apdu_kat.XXXXXX)"
cc -std=c11 -O1 -Wall -Wextra \
   -I "$ALIRO/include" -I "$ALIRO/src" \
   "$HERE/test_aliro_apdu.c" "$ALIRO/src/aliro_apdu.c" -o "$ABIN"
"$ABIN"
rm -f "$ABIN"

echo
echo "== host: aliro_stepup Access-Document codec + §7.4 verifier KAT =="
SBIN="$(mktemp -t aliro_stepup_kat.XXXXXX)"
cc -std=c11 -O1 -Wall -Wextra \
   -I "$HERE" -I "$ALIRO/include" -I "$ALIRO/src" \
   "$HERE/test_aliro_stepup.c" \
   "$ALIRO/src/aliro_stepup.c" "$ALIRO/src/aliro_stepup_parse.c" \
   "$ALIRO/src/aliro_hash.c" "$ALIRO/src/aliro_crypto.c" \
   "$HERE/aliro_prim_host.c" -o "$SBIN"
"$SBIN"
rm -f "$SBIN"

echo
echo "== host: aliro_prov identity/trust KAT =="
PBIN="$(mktemp -t aliro_prov_kat.XXXXXX)"
cc -std=c11 -O1 -Wall -Wextra \
   -I "$ALIRO/include" -I "$ALIRO/src" \
   "$HERE/test_aliro_prov.c" "$ALIRO/src/aliro_prov.c" -o "$PBIN"
"$PBIN"
rm -f "$PBIN"

echo
echo "== host: aliro_lat walk-up trace (gate on + gate off) =="
# _POSIX_C_SOURCE: woz_port.h's host woz_uptime_us needs clock_gettime /
# CLOCK_MONOTONIC, which strict -std=c11 hides on glibc (macOS exposes them
# regardless). A -D lands before every include, so ordering is safe.
WOZ_PORT_INC="$HERE/../../../modules/woz_port/include"
TBIN="$(mktemp -t aliro_lat.XXXXXX)"
cc -std=c11 -O1 -Wall -Wextra \
   -D_POSIX_C_SOURCE=200809L \
   -DWOZ_PORT_HOST -DCONFIG_ALIRO_LAT_TRACE=1 \
   -I "$ALIRO/include" -I "$WOZ_PORT_INC" \
   "$HERE/test_aliro_lat.c" "$ALIRO/src/aliro_lat.c" -o "$TBIN"
"$TBIN"
cc -std=c11 -O1 -Wall -Wextra \
   -D_POSIX_C_SOURCE=200809L \
   -DWOZ_PORT_HOST \
   -I "$ALIRO/include" -I "$WOZ_PORT_INC" \
   "$HERE/test_aliro_lat.c" "$ALIRO/src/aliro_lat.c" -o "$TBIN"
"$TBIN"
rm -f "$TBIN"

echo
echo "== host: aliro_reader engine walk-up (scripted phone) =="
# The reader engine end-to-end: a scripted phone drives AUTH0/AUTH1/EXCHANGE/
# AP-Completed against the real state machine + codec + key schedule, with the
# BLE transport, ranging adapter and NVS backend as recording doubles and the
# fake-EC prim double standing in for the curve (see aliro_prim_host.c).
# -Wno-unused-variable/-function: the host LOG no-ops orphan the unit's rc/
# diagnostic locals; the test file itself stays warning-clean.
RBIN="$(mktemp -t aliro_reader.XXXXXX)"
cc -std=c11 -O1 -Wall -Wextra \
   -Wno-unused-variable -Wno-unused-function \
   -D_POSIX_C_SOURCE=200809L -DWOZ_PORT_HOST \
   -I "$ALIRO/include" -I "$ALIRO/src" -I "$WOZ_PORT_INC" \
   "$HERE/test_aliro_reader.c" \
   "$ALIRO/src/aliro_reader.c" "$ALIRO/src/aliro_apdu.c" \
   "$ALIRO/src/aliro_crypto.c" "$ALIRO/src/aliro_hash.c" \
   "$ALIRO/src/aliro_prov.c" \
   "$HERE/aliro_prim_host.c" -o "$RBIN"
"$RBIN"
rm -f "$RBIN"

echo
echo "== host: aliro_ranging M1-M4 session glue =="
# The ranging-setup glue against recording doubles of the engine (cherry/
# adapter/session), the BLE transport and the woz_uwb facade; the BleSK
# sealing in the transmit callback is real crypto, opened by the test with
# the mirrored device-direction GCM.
UWB_SRC="$HERE/../../../modules/woz_uwb/src"
GBIN="$(mktemp -t aliro_ranging.XXXXXX)"
cc -std=c11 -O1 -Wall -Wextra \
   -D_POSIX_C_SOURCE=200809L -DWOZ_PORT_HOST \
   -I "$ALIRO/include" -I "$ALIRO/src" -I "$WOZ_PORT_INC" \
   -I "$UWB_SRC/facade" -I "$UWB_SRC/aliro/include" \
   "$HERE/test_aliro_ranging.c" \
   "$ALIRO/src/aliro_ranging.c" "$ALIRO/src/aliro_crypto.c" \
   "$ALIRO/src/aliro_hash.c" \
   "$HERE/aliro_prim_host.c" -o "$GBIN"
"$GBIN"
rm -f "$GBIN"

echo
echo "== host: bolt-state LED policy =="
MATTER_MAIN="$HERE/../apps/matter-lock/main"
LBIN="$(mktemp -t lock_led.XXXXXX)"
cc -std=c11 -O1 -Wall -Wextra \
   -I "$MATTER_MAIN" \
   "$HERE/test_lock_led.c" "$MATTER_MAIN/lock_led.c" -o "$LBIN"
"$LBIN"
rm -f "$LBIN"

echo
echo "== target: port build + link-seam guard =="
bash "$HERE/verify_port.sh"
