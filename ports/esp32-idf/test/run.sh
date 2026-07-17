#!/usr/bin/env bash
#
# Test entry point for the ESP32 port. Three layers, all hardware-free:
#   - test_compat_shim:  fast host unit test of the pure compat headers.
#   - test_aliro_crypto: host KAT of the Aliro key-schedule core (SHA-256/KDF),
#                        compiled from the same source as the target.
#   - verify_port.sh:    on-target build + --wrap seam + exclusion guard (needs
#                        the ESP-IDF env; skips cleanly without it).
#
# On-target functional tests (Unity on the DW3000 SPI/IRQ path) are deferred:
# they need the DWM3000EVB wired up. See ../BRINGUP.md.
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"

echo "== host: compat shim unit test =="
BIN="$(mktemp -t woz_compat_shim.XXXXXX)"
trap 'rm -f "$BIN" "${CBIN:-}"' EXIT
cc -std=c11 -O1 -Wall -Wextra \
   -I "$HERE/../components/woz_uwb/compat" \
   "$HERE/test_compat_shim.c" -o "$BIN"
"$BIN"

echo
echo "== host: aliro_crypto key-schedule KAT =="
CRYPTO="$HERE/../components/aliro_crypto"
CBIN="$(mktemp -t aliro_crypto_kat.XXXXXX)"
cc -std=c11 -O1 -Wall -Wextra \
   -I "$CRYPTO/include" -I "$CRYPTO/src" \
   "$HERE/test_aliro_crypto.c" \
   "$CRYPTO/src/aliro_hash.c" "$CRYPTO/src/aliro_crypto.c" \
   "$HERE/aliro_prim_host.c" -o "$CBIN"
"$CBIN"

echo
echo "== host: aliro_apdu wire-codec KAT =="
READER="$HERE/../components/aliro_reader"
ABIN="$(mktemp -t aliro_apdu_kat.XXXXXX)"
cc -std=c11 -O1 -Wall -Wextra \
   -I "$READER" \
   "$HERE/test_aliro_apdu.c" "$READER/aliro_apdu.c" -o "$ABIN"
"$ABIN"
rm -f "$ABIN"

echo
echo "== target: port build + link-seam guard =="
bash "$HERE/verify_port.sh"
