#!/usr/bin/env python3
"""Apply the battery-status feature to a fresh upstream checkout.

This migration is intentionally independent from unlock policy. It adds only a
battery module, one Kconfig source hook, one optional source line, and a Matter
integration seam that can be reviewed as a standalone upstream change.
"""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
APP = ROOT / "apps" / "dwm3001cdk-lock"
SRC = APP / "src"

BATTERY_H = r'''/* SPDX-License-Identifier: ISC */
#pragma once
#include <stdint.h>

/* Return battery state of charge as 0..100 percent. */
uint8_t battery_status_percent(void);
'''

BATTERY_C = r'''/* SPDX-License-Identifier: ISC */
#include "battery_status.h"

/*
 * Board-specific battery source. Kept outside the Matter implementation on
 * purpose: Matter consumes a percentage and does not need to know how the
 * DWM3001CDK obtains it. The first integration reports a stable 100% value;
 * the VDD/chemistry mapping can evolve here without touching upstream Matter.
 */
uint8_t battery_status_percent(void)
{
    return 100u;
}
'''

KCONFIG = r'''config CUSTOM_BATTERY_STATUS
    bool "Matter battery percentage reporting"
    depends on ULTRAWIDELOCK_MATTER_BLE
    default n
    help
      Adds the downstream battery status extension. When disabled, no battery
      source code or Matter battery hooks are compiled.
'''

OVERLAY = 'CONFIG_CUSTOM_BATTERY_STATUS=y\n'


def replace_once(path: Path, old: str, new: str):
    text = path.read_text()
    if new in text:
        return
    if old not in text:
        raise SystemExit(f"anchor not found in {path}: {old[:100]!r}")
    path.write_text(text.replace(old, new, 1))


(SRC / "battery_status.h").write_text(BATTERY_H)
(SRC / "battery_status.c").write_text(BATTERY_C)
(APP / "Kconfig.battery-status").write_text(KCONFIG)
(APP / "overlay-battery-status.conf").write_text(OVERLAY)

# One Kconfig hook, independent from every other custom feature. `rsource`
# resolves relative to this Kconfig file; plain `source` resolves from Zephyr's
# global srctree and therefore cannot find the app-local fragment.
kcfg = APP / "Kconfig"
replace_once(kcfg,
             'source "Kconfig.zephyr"\n',
             'rsource "Kconfig.battery-status"\n\nsource "Kconfig.zephyr"\n')

# Compile the battery implementation only when explicitly enabled.
cmake = APP / "CMakeLists.txt"
anchor = 'target_sources_ifdef(CONFIG_ULTRAWIDELOCK_STATUS_LED app PRIVATE src/status_led.c)\n'
replace_once(cmake, anchor,
             anchor + 'target_sources_ifdef(CONFIG_CUSTOM_BATTERY_STATUS app PRIVATE src/battery_status.c)\n')

print("Applied standalone battery-status feature skeleton.")
print("This branch intentionally contains no unlock-guard code.")
