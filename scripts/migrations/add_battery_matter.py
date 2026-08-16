#!/usr/bin/env python3
"""Add Matter Power Source presentation for the standalone battery feature.

Run after add_battery_status.py.  Kept separate so the board-side measurement
and the Matter data-model change can be reviewed/rebased independently.
"""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
APP = ROOT / "apps" / "dwm3001cdk-lock"
SRC = APP / "src"
HDR = ROOT / "modules" / "ultrawidelock_matter" / "include" / "matter_clusters.h"
CLUSTERS = ROOT / "modules" / "ultrawidelock_matter" / "src" / "matter_clusters.c"
COMMISSION = SRC / "matter_commission.c"


def replace_once(path: Path, old: str, new: str):
    text = path.read_text()
    if new in text:
        return
    if old not in text:
        raise SystemExit(f"anchor not found in {path}: {old[:100]!r}")
    path.write_text(text.replace(old, new, 1))

# Public Matter constants + optional device-info fields.  BatPercentRemaining is
# encoded in half-percent units by Matter (0..200).
replace_once(HDR,
    '#define MATTER_CLUSTER_DOOR_LOCK               0x0101u\n',
    '#define MATTER_CLUSTER_DOOR_LOCK               0x0101u\n'
    '#ifdef CONFIG_CUSTOM_BATTERY_STATUS\n'
    '#define MATTER_CLUSTER_POWER_SOURCE            0x002Fu\n'
    '#define MATTER_ATTR_PS_STATUS                   0x0000u\n'
    '#define MATTER_ATTR_PS_ORDER                    0x0001u\n'
    '#define MATTER_ATTR_PS_DESCRIPTION              0x0002u\n'
    '#define MATTER_ATTR_PS_BAT_PERCENT_REMAINING    0x000Cu\n'
    '#define MATTER_ATTR_PS_BAT_CHARGE_LEVEL         0x000Eu\n'
    '#endif\n')

replace_once(HDR,
    '\tuint8_t lock_state;\n',
    '\tuint8_t lock_state;\n'
    '#ifdef CONFIG_CUSTOM_BATTERY_STATUS\n'
    '\tuint8_t battery_percent; /* 0..100; Matter encoder converts to 0..200 */\n'
    '#endif\n')

# Keep the feature on the lock endpoint: Home then associates battery state with
# the lock accessory instead of creating an unrelated third accessory tile.
replace_once(CLUSTERS,
    '\tMATTER_CLUSTER_APPROACH_DIRECTION,\n};\n',
    '\tMATTER_CLUSTER_APPROACH_DIRECTION,\n'
    '#ifdef CONFIG_CUSTOM_BATTERY_STATUS\n'
    '\tMATTER_CLUSTER_POWER_SOURCE,\n'
    '#endif\n'
    '};\n')

replace_once(CLUSTERS,
    '\t\treturn cluster == MATTER_CLUSTER_DESCRIPTOR ||\n\t\t       cluster == MATTER_CLUSTER_DOOR_LOCK ||\n\t\t       cluster == MATTER_CLUSTER_APPROACH_DIRECTION;\n',
    '\t\treturn cluster == MATTER_CLUSTER_DESCRIPTOR ||\n\t\t       cluster == MATTER_CLUSTER_DOOR_LOCK ||\n\t\t       cluster == MATTER_CLUSTER_APPROACH_DIRECTION\n'
    '#ifdef CONFIG_CUSTOM_BATTERY_STATUS\n'
    '\t\t       || cluster == MATTER_CLUSTER_POWER_SOURCE\n'
    '#endif\n'
    '\t\t       ;\n')

# Attribute support and encoding are inserted before the existing DoorLock-only
# fallback.  Global attributes are intentionally omitted in this first narrow
# integration; Home only needs the battery values for the accessory UI.
needle = '\t\tif (cluster != MATTER_CLUSTER_DOOR_LOCK) {\n\t\t\treturn MATTER_IM_STATUS_UNSUPPORTED_CLUSTER;\n\t\t}\n'
insert = '''#ifdef CONFIG_CUSTOM_BATTERY_STATUS
		if (cluster == MATTER_CLUSTER_POWER_SOURCE) {
			switch (attribute) {
			case MATTER_ATTR_PS_STATUS:
			case MATTER_ATTR_PS_ORDER:
			case MATTER_ATTR_PS_DESCRIPTION:
			case MATTER_ATTR_PS_BAT_PERCENT_REMAINING:
			case MATTER_ATTR_PS_BAT_CHARGE_LEVEL:
				return MATTER_IM_STATUS_SUCCESS;
			default:
				return MATTER_IM_STATUS_UNSUPPORTED_ATTRIBUTE;
			}
		}
#endif
''' + needle
replace_once(CLUSTERS, needle, insert)

# Encode Power Source values before Approach Direction / DoorLock handling.
needle2 = '\tif (cluster == MATTER_CLUSTER_APPROACH_DIRECTION) {\n'
insert2 = '''#ifdef CONFIG_CUSTOM_BATTERY_STATUS
	if (cluster == MATTER_CLUSTER_POWER_SOURCE) {
		switch (attribute) {
		case MATTER_ATTR_PS_STATUS:
			(void)matter_tlv_put_u64(w, tag, 0u); /* Active */
			return;
		case MATTER_ATTR_PS_ORDER:
			(void)matter_tlv_put_u64(w, tag, 0u);
			return;
		case MATTER_ATTR_PS_DESCRIPTION:
			(void)matter_tlv_put_utf8(w, tag, "Battery", 7u);
			return;
		case MATTER_ATTR_PS_BAT_PERCENT_REMAINING:
			(void)matter_tlv_put_u64(w, tag, (uint64_t)info->battery_percent * 2u);
			return;
		case MATTER_ATTR_PS_BAT_CHARGE_LEVEL:
			(void)matter_tlv_put_u64(w, tag, info->battery_percent <= 10u ? 2u :
						 info->battery_percent <= 20u ? 1u : 0u);
			return;
		default:
			return;
		}
	}
#endif
''' + needle2
replace_once(CLUSTERS, needle2, insert2)

# Board-specific measurement stays in the app; Matter receives only a percent.
replace_once(COMMISSION,
    '#include "status_led.h" /* the lock LED; a tile tap has to move it too */\n',
    '#include "status_led.h" /* the lock LED; a tile tap has to move it too */\n'
    '#ifdef CONFIG_CUSTOM_BATTERY_STATUS\n'
    '#include "battery_status.h"\n'
    '#endif\n')
replace_once(COMMISSION,
    'int matter_commission_init(void)\n{\n',
    'int matter_commission_init(void)\n{\n'
    '#ifdef CONFIG_CUSTOM_BATTERY_STATUS\n'
    '\ts_info.battery_percent = battery_status_percent();\n'
    '#endif\n')

print("Applied standalone Matter Power Source integration.")
