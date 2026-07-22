#!/usr/bin/env bash
#
# Line coverage for our own (asxeem) host-testable code, via clang source-based
# coverage. Instruments every host suite in the repo — the woz_uwb KAT suite
# (same sources as run.sh, see sources.sh) and the shared-core suites mirrored
# from ports/esp32/test/run.sh — merges their profiles into one report.
#
# Sources that never enter a host build are not hidden: this script discovers
# them (find over modules/ + ports/, no hand-list to go stale) and the report
# prints each as a 0% row — tagged "untested" when nothing platform-bound
# blocks a host build, "target-only" when it needs Zephyr/ESP-IDF/PSA/NimBLE
# or silicon. Headers with static-inline bodies join the accounting: llvm
# attributes their lines when an instrumented TU instantiates them, and the
# rest fall through to the 0% table. Non-C surfaces (python, web pages, the
# nRF add-on patches, shell tooling) are listed in a block of their own.
# Excluded entirely: deps/ + workspace/ (fetched upstream) and */test/
# harnesses.
#
# CI (host-tests.yml) enforces the line floor on summary.json, which spans the
# instrumented files only; the terminal table's closing "all our code" total
# additionally folds in the 0% rows.
#
# Artifacts under build/coverage/ (build/ is gitignored). The instrumented
# suites may report test failures; coverage is still generated (execution is
# what counts). A crash (signal) would abort, and should.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
. "$ROOT/tests/host/sources.sh"

OUT="$ROOT/build/coverage"
BIN="$OUT/host_test_cov"
mkdir -p "$OUT"
rm -f "$OUT"/*.profraw # stale profiles from removed suites break the merge

# Apple toolchains front the LLVM tools with xcrun; Linux has them on PATH bare.
llvm_tool() { if command -v xcrun >/dev/null 2>&1; then xcrun "$@"; else "$@"; fi; }

# The instrumentation flags are clang-only: macOS cc is clang, Linux CI sets CC=clang.
# -w: coverage is not a lint gate (run.sh / the Zephyr build are). Errors still fail.
cov_cc() {
	"${CC:-cc}" -std=c11 -O0 -g -w \
		-fprofile-instr-generate -fcoverage-mapping "$@"
}

# --- suite 1: the woz_uwb host KAT suite (same sources as run.sh) -----------
cov_cc "${DEFS[@]}" "${INCS[@]}" \
	"${TEST_SRCS[@]}" "${SHIM_SRCS[@]}" "${UNIT_SRCS[@]}" -o "$BIN"
LLVM_PROFILE_FILE="$OUT/host.profraw" "$BIN" >"$OUT/run.log" 2>&1 || true

# --- suite 2..n: shared-core host KATs (mirror of ports/esp32/test/run.sh) --
ET="$ROOT/ports/esp32/test"
ALIRO="$ROOT/modules/woz_aliro"
LOCK_MAIN="$ROOT/ports/esp32/apps/matter-lock/main"

# Units those suites exercise; joins UNIT_SRCS in the coverage denominator.
CORE_UNIT_SRCS=(
	"$ALIRO/src/aliro_hash.c"
	"$ALIRO/src/aliro_crypto.c"
	"$ALIRO/src/aliro_advtag.c"
	"$ALIRO/src/aliro_apdu.c"
	"$ALIRO/src/aliro_stepup.c"
	"$ALIRO/src/aliro_stepup_parse.c"
	"$ALIRO/src/aliro_prov.c"
	"$ALIRO/src/aliro_lat.c"
	"$ALIRO/src/aliro_reader.c"
	"$ALIRO/src/aliro_ranging.c"
	"$LOCK_MAIN/lock_led.c"
)

OBJS=()
run_suite() { # <name> <bin>: run one instrumented suite into its own profile
	LLVM_PROFILE_FILE="$OUT/$1.profraw" "$2" >>"$OUT/run.log" 2>&1 || true
	OBJS+=(-object "$2")
}

cov_cc -I"$ALIRO/include" -I"$ALIRO/src" \
	"$ET/test_aliro_crypto.c" \
	"$ALIRO/src/aliro_hash.c" "$ALIRO/src/aliro_crypto.c" "$ALIRO/src/aliro_advtag.c" \
	"$ET/aliro_prim_host.c" -o "$OUT/cov_crypto"
run_suite crypto "$OUT/cov_crypto"

cov_cc -I"$ALIRO/include" -I"$ALIRO/src" \
	"$ET/test_aliro_apdu.c" "$ALIRO/src/aliro_apdu.c" -o "$OUT/cov_apdu"
run_suite apdu "$OUT/cov_apdu"

cov_cc -I"$ET" -I"$ALIRO/include" -I"$ALIRO/src" \
	"$ET/test_aliro_stepup.c" \
	"$ALIRO/src/aliro_stepup.c" "$ALIRO/src/aliro_stepup_parse.c" \
	"$ALIRO/src/aliro_hash.c" "$ALIRO/src/aliro_crypto.c" \
	"$ET/aliro_prim_host.c" -o "$OUT/cov_stepup"
run_suite stepup "$OUT/cov_stepup"

cov_cc -I"$ALIRO/include" -I"$ALIRO/src" \
	"$ET/test_aliro_prov.c" "$ALIRO/src/aliro_prov.c" -o "$OUT/cov_prov"
run_suite prov "$OUT/cov_prov"

# Only the trace-on lat build is instrumented: the gate-off variant maps the
# same lines differently and the two profiles would not merge cleanly.
cov_cc -D_POSIX_C_SOURCE=200809L -DWOZ_PORT_HOST -DCONFIG_ALIRO_LAT_TRACE=1 \
	-I"$ALIRO/include" -I"$ROOT/modules/woz_port/include" \
	"$ET/test_aliro_lat.c" "$ALIRO/src/aliro_lat.c" -o "$OUT/cov_lat"
run_suite lat "$OUT/cov_lat"

cov_cc -I"$LOCK_MAIN" \
	"$ET/test_lock_led.c" "$LOCK_MAIN/lock_led.c" -o "$OUT/cov_led"
run_suite led "$OUT/cov_led"

cov_cc -D_POSIX_C_SOURCE=200809L -DWOZ_PORT_HOST \
	-I"$ALIRO/include" -I"$ALIRO/src" -I"$ROOT/modules/woz_port/include" \
	"$ET/test_aliro_reader.c" \
	"$ALIRO/src/aliro_reader.c" "$ALIRO/src/aliro_apdu.c" \
	"$ALIRO/src/aliro_crypto.c" "$ALIRO/src/aliro_hash.c" \
	"$ALIRO/src/aliro_prov.c" \
	"$ET/aliro_prim_host.c" -o "$OUT/cov_reader"
run_suite reader "$OUT/cov_reader"

cov_cc -D_POSIX_C_SOURCE=200809L -DWOZ_PORT_HOST \
	-I"$ALIRO/include" -I"$ALIRO/src" -I"$ROOT/modules/woz_port/include" \
	-I"$ROOT/modules/woz_uwb/src/facade" -I"$ROOT/modules/woz_uwb/src/aliro/include" \
	"$ET/test_aliro_ranging.c" \
	"$ALIRO/src/aliro_ranging.c" "$ALIRO/src/aliro_crypto.c" \
	"$ALIRO/src/aliro_hash.c" \
	"$ET/aliro_prim_host.c" -o "$OUT/cov_ranging"
run_suite ranging "$OUT/cov_ranging"

# Header-inline logic (woz_port.h et al.) is exercised by the port-headers
# unit test; instrumenting it attributes those lines to the headers below.
cov_cc -I"$ROOT/modules/woz_port/include" -I"$ROOT/modules/woz_uwb/src/facade" \
	"$ET/test_port_headers.c" -o "$OUT/cov_hdrs"
run_suite hdrs "$OUT/cov_hdrs"

llvm_tool llvm-profdata merge -sparse "$OUT"/*.profraw -o "$OUT/host.profdata"

ALL_UNIT_SRCS=("${UNIT_SRCS[@]}" "${CORE_UNIT_SRCS[@]}")

# Our headers, all of them: llvm-cov attributes inline-function coverage to
# the header wherever an instrumented TU instantiated it, and silently skips
# paths with no coverage mapping, so the whole set can be passed.
HDR_SRCS=()
while IFS= read -r h; do
	HDR_SRCS+=("$ROOT/$h")
done < <(cd "$ROOT" && find modules ports -name '*.h' ! -path '*/test/*' | LC_ALL=C sort)

# Browsable HTML, restricted to the units under test.
llvm_tool llvm-cov show "$BIN" "${OBJS[@]}" -instr-profile="$OUT/host.profdata" \
	"${ALL_UNIT_SRCS[@]}" \
	-format=html -output-dir="$OUT/html" \
	-show-line-counts-or-regions -show-branches=count >/dev/null

# Machine-readable summary for the terminal table (and the CI floor).
llvm_tool llvm-cov export "$BIN" "${OBJS[@]}" -instr-profile="$OUT/host.profdata" \
	-summary-only "${ALL_UNIT_SRCS[@]}" "${HDR_SRCS[@]}" >"$OUT/summary.json"

# --- everything of ours that never enters a host build ----------------------
# Discovered, not hand-listed, so a new source file shows up here on its own.
# Headers join the scan when they hold static-inline bodies; the report drops
# any candidate llvm-cov already measured, so only truly unbuilt code remains.
UNBUILT_TSV="$OUT/unbuilt.tsv"
: >"$UNBUILT_TSV"
PLATFORM_RE='#include [<"](zephyr/|esp_|nimble/|host/|freertos/|nvs|mbedtls/|psa/|deca_|app/|driver/|crypto/|lib/)'
while IFS= read -r rel; do
	f="$ROOT/$rel"
	for u in "${ALL_UNIT_SRCS[@]}"; do
		[ "$f" = "$u" ] && continue 2
	done
	tag=untested
	if grep -q -E "$PLATFORM_RE" "$f"; then
		tag=target-only
	fi
	printf '%s\t%s\n' "$rel" "$tag" >>"$UNBUILT_TSV"
done < <(cd "$ROOT" && {
	find modules ports \( -name '*.c' -o -name '*.cc' -o -name '*.cpp' \) \
		! -path '*/test/*' ! -path '*/managed_components/*'
	find modules ports -name '*.h' ! -path '*/test/*' \
		-exec grep -l 'static inline' {} +
} | LC_ALL=C sort)

# --- surfaces beyond C: listed for visibility, not instrumented --------------
SURFACES_TSV="$OUT/surfaces.tsv"
: >"$SURFACES_TSV"
surf() { printf '%s\t%s\t%s\n' "$1" "$2" "$3" >>"$SURFACES_TSV"; }
loc() { wc -l <"$ROOT/$1" | tr -d ' '; }

surf "tools/aliro_lab.py" "$(loc tools/aliro_lab.py)" \
	"tested by tests/host/test_aliro_lab.py — lines unmeasured"
surf "integration/homeassistant/aliro_mqtt_bridge.py" \
	"$(loc integration/homeassistant/aliro_mqtt_bridge.py)" "no tests"
surf "scripts/flash_html.py" "$(loc scripts/flash_html.py)" "no tests"
surf "web-twin/index.html" "$(loc web-twin/index.html)" \
	"constants drift-gated in CI; JS logic untested"
surf "web-flasher/index.html" "$(loc web-flasher/index.html)" "no tests"
npatch="$(find "$ROOT/ports/nrf5340dk/patches" -name '*.patch' | wc -l | tr -d ' ')"
nadded="$(find "$ROOT/ports/nrf5340dk/patches" -name '*.patch' -exec cat {} + |
	grep -c '^+[^+]')"
surf "ports/nrf5340dk/patches/ ($npatch patches)" "$nadded" \
	"our code inside the Nordic add-on — target-only"
ndocs="$(find "$ROOT/tools" -name 'docs_*.py' | wc -l | tr -d ' ')"
ndocsl="$(find "$ROOT/tools" -name 'docs_*.py' -exec cat {} + | wc -l | tr -d ' ')"
surf "tools/docs_*.py ($ndocs files)" "$ndocsl" \
	"docs pipeline — exercised by make docs, no unit tests"
nsh="$(cd "$ROOT" && find scripts release tests/tooling -name '*.sh' | wc -l | tr -d ' ')"
nshl="$(cd "$ROOT" && find scripts release tests/tooling -name '*.sh' -exec cat {} + |
	wc -l | tr -d ' ')"
surf "scripts/ + release/ shell ($nsh scripts)" "$nshl" \
	"shellcheck-gated; tests/tooling covers ws-seed + patch drift"

python3 "$ROOT/tests/host/coverage_report.py" \
	"$OUT/summary.json" "$OUT/html/index.html" "$UNBUILT_TSV" "$SURFACES_TSV"

# Surface a failing suite without aborting the coverage report.
if ! grep -q "RESULT: PASS" "$OUT/run.log"; then
	echo "    note: suite did not report PASS — see $OUT/run.log"
fi
