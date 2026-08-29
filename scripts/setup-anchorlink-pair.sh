#!/usr/bin/env bash
# Build and flash a DWM3001CDK lock + role-2 satellite pair.

set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
lock_probe=""
sat_probe=""
skip_build=0

usage() {
  cat <<'EOF'
Usage: scripts/setup-anchorlink-pair.sh [options]

Builds the configured anchorlink installation image with the Matter client and
release-size profile enabled, and the DWM3001CDK Thread satellite as role 2,
then guides you through flashing both boards.

Options:
  --lock-probe ID       Probe ID for the inside lock board
  --satellite-probe ID  Probe ID for the outside satellite board
  --skip-build          Reuse existing build output
  -h, --help            Show this help

With no probe IDs, disconnect both boards and follow the connection prompts.
Supplying both IDs allows both boards to remain connected throughout.
EOF
}

while (($#)); do
  case "$1" in
    --lock-probe)
      [[ $# -ge 2 ]] || { echo "Missing value for --lock-probe" >&2; exit 2; }
      lock_probe="$2"
      shift 2
      ;;
    --satellite-probe)
      [[ $# -ge 2 ]] || { echo "Missing value for --satellite-probe" >&2; exit 2; }
      sat_probe="$2"
      shift 2
      ;;
    --skip-build)
      skip_build=1
      shift
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "Unknown option: $1" >&2
      usage >&2
      exit 2
      ;;
  esac
done

if [[ -n "$lock_probe" || -n "$sat_probe" ]]; then
  if [[ -z "$lock_probe" || -z "$sat_probe" ]]; then
    echo "Pass both --lock-probe and --satellite-probe, or neither." >&2
    exit 2
  fi
  if [[ "$lock_probe" == "$sat_probe" ]]; then
    echo "Lock and satellite probe IDs must differ." >&2
    exit 2
  fi
fi

for tool in make probe-rs; do
  command -v "$tool" >/dev/null 2>&1 || {
    echo "Missing required tool: $tool" >&2
    exit 1
  }
done

cd "$repo_root"
branch="$(git branch --show-current 2>/dev/null || true)"
if [[ "$branch" != "main" ]]; then
  echo "Note: current branch is '${branch:-detached}', not 'main'."
  echo "The script will use the current checkout and will not switch branches."
  echo
fi

sat_build="build/satellite-decawave_dwm3001cdk-thread-role2"
lock_build="build/cdk-anchorlink-bench"
setup_build="$repo_root/build/setup-anchorlink-pair"
sat_role_conf="$setup_build/satellite-role2.conf"
lock_install_conf="$setup_build/lock-installation.conf"

# Keep installation-specific policy generated under build/. This makes the
# setup script usable from a clean main checkout without adding product-local
# overlays or changing the shared makefiles.
mkdir -p "$setup_build"
cat >"$sat_role_conf" <<'EOF'
CONFIG_ULTRAWIDELOCK_ANCHOR_ROLE=2
EOF
cat >"$lock_install_conf" <<'EOF'
CONFIG_ULTRAWIDELOCK_ANCHOR_BASELINE_MM=1400
CONFIG_ULTRAWIDELOCK_ANCHOR_BASELINE_2_MM=1400
CONFIG_ULTRAWIDELOCK_ANCHOR_BOUNDARY_BIAS_MM=1150
EOF

sat_conf="overlay-thread.conf;$sat_role_conf"
lock_conf="overlay-thread.conf;overlay-latch.conf;overlays/bench-2resp.conf;overlay-anchorlink.conf;overlay-release.conf;overlay-client.conf;overlays/bench-anchorlink.conf;$lock_install_conf;overlay-lto.conf"

if ((skip_build == 0)); then
  echo "[1/4] Building outside satellite (DWM3001CDK, Thread, role 2)..."
  make sat-build \
    SAT_BOARD=decawave_dwm3001cdk \
    SAT_THREAD=1 \
    SAT_THREAD_CONF="$sat_conf" \
    SAT_BUILD="$sat_build"

  echo "[2/4] Building inside lock (BENCH=1 CLIENT=1 RELEASE=1)..."
  make anchorlink BENCH=1 CLIENT=1 RELEASE=1 \
    CDK_ANCHORLINK_CONF="$lock_conf"
else
  echo "Reusing existing build output (--skip-build)."
fi

sat_config="$sat_build/satellite/zephyr/.config"
lock_config="$lock_build/dwm3001cdk-lock/zephyr/.config"
[[ -f "$sat_config" ]] || { echo "Missing satellite build: $sat_config" >&2; exit 1; }
[[ -f "$lock_config" ]] || { echo "Missing lock build: $lock_config" >&2; exit 1; }

grep -qx 'CONFIG_ULTRAWIDELOCK_ANCHOR_ROLE=2' "$sat_config" || {
  echo "Refusing to flash: satellite image is not role 2." >&2
  exit 1
}
grep -qx 'CONFIG_ULTRAWIDELOCK_ANCHOR_BASELINE_2_MM=1400' "$lock_config" || {
  echo "Refusing to flash: lock role-2 baseline is not 1400 mm." >&2
  exit 1
}
grep -qx 'CONFIG_ULTRAWIDELOCK_ANCHOR_BOUNDARY_BIAS_MM=1150' "$lock_config" || {
  echo "Refusing to flash: lock boundary bias is not 1150 mm." >&2
  exit 1
}
grep -qx 'CONFIG_ULTRAWIDELOCK_MATTER_CLIENT=y' "$lock_config" || {
  echo "Refusing to flash: lock image was not built with CLIENT=1." >&2
  exit 1
}
grep -qx 'CONFIG_LOG_DEFAULT_LEVEL=1' "$lock_config" || {
  echo "Refusing to flash: lock image does not contain the RELEASE=1 size profile." >&2
  exit 1
}

detect_single_probe() {
  local label="$1" output probes count
  while true; do
    output="$(probe-rs list)"
    probes="$(printf '%s\n' "$output" | sed -nE 's/^\[[0-9]+\]: .* -- ([^ ]+) \(.*/\1/p')"
    count="$(printf '%s\n' "$probes" | sed '/^$/d' | wc -l | tr -d ' ')"
    if [[ "$count" == "1" ]]; then
      printf '%s\n' "$probes"
      return 0
    fi
    echo "Expected exactly one connected probe for $label; found $count." >&2
    printf '%s\n' "$output" >&2
    read -r -p "Fix the USB connections, then press Enter to retry... " </dev/tty
  done
}

if [[ -z "$sat_probe" ]]; then
  echo
  echo "Disconnect both boards. Connect ONLY the OUTSIDE SATELLITE board."
  read -r -p "Press Enter when it is connected... " </dev/tty
  sat_probe="$(detect_single_probe satellite)"
fi

echo "[3/4] Flashing outside satellite on $sat_probe..."
make sat-flash \
  SAT_BOARD=decawave_dwm3001cdk \
  SAT_THREAD=1 \
  SAT_BUILD="$sat_build" \
  CDK_PROBE="$sat_probe"

if [[ -z "$lock_probe" ]]; then
  echo
  echo "Disconnect the satellite. Connect ONLY the INSIDE LOCK board."
  read -r -p "Press Enter when it is connected... " </dev/tty
  lock_probe="$(detect_single_probe lock)"
fi

echo "[4/4] Flashing inside lock on $lock_probe (settings are preserved)..."
make flash CDK_BUILD="$lock_build" CDK_PROBE="$lock_probe"

cat <<EOF

Pair installation complete.

Physical placement:
  - lock      : inside
  - satellite : outside
  - configured anchor separation: 1400 mm
  - boundary bias: 1150 mm (frontier about 125 mm before the lock)
  - Matter client: enabled
  - release-size profile: enabled

Reconnect both boards before testing. Monitor commands:

  make monitor CDK_RTT_BUILD=$lock_build CDK_PROBE=$lock_probe

  make sat-monitor SAT_BOARD=decawave_dwm3001cdk SAT_THREAD=1 \\
    SAT_BUILD=$sat_build SAT_PROBE=$sat_probe
EOF
