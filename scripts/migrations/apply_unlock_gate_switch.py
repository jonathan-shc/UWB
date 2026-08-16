#!/usr/bin/env python3
"""Apply the standalone owner-controlled Matter unlock-gate integration.

This script intentionally touches only the minimal upstream-oriented seams:
- app Kconfig includes one local fragment;
- app CMake conditionally adds one isolated source file;
- Matter cluster model gains endpoint 3 On/Off;
- main checks one gate function before passive approach unlocks.

No battery dependency. Safe to carry/cherry-pick as a standalone feature patch.
"""
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[2]
APP = ROOT / "apps" / "dwm3001cdk-lock"
SRC = APP / "src"
KCFG = APP / "Kconfig"
CMAKE = APP / "CMakeLists.txt"
MAIN = SRC / "main.c"
HDR = ROOT / "modules" / "ultrawidelock_matter" / "include" / "matter_clusters.h"
CLU = ROOT / "modules" / "ultrawidelock_matter" / "src" / "matter_clusters.c"
MCOMM = SRC / "matter_commission.c"


def insert_once(path: Path, anchor: str, addition: str, after=True):
    text = path.read_text()
    if addition in text:
        return
    if anchor not in text:
        raise SystemExit(f"anchor not found in {path}: {anchor[:100]!r}")
    repl = anchor + addition if after else addition + anchor
    path.write_text(text.replace(anchor, repl, 1))

# Kconfig and CMake: one small, obvious hook each.
insert_once(KCFG, 'source "Kconfig.zephyr"\n', 'rsource "Kconfig.unlock-gate"\n\n', after=False)
insert_once(CMAKE,
            'target_sources_ifdef(CONFIG_ULTRAWIDELOCK_STATUS_LED app PRIVATE src/status_led.c)\n',
            'target_sources_ifdef(CONFIG_ULTRAWIDELOCK_UNLOCK_GATE_SWITCH app PRIVATE src/unlock_gate.c)\n')

# Main: include the isolated module only when enabled.
insert_once(MAIN, '#include "status_led.h"\n',
            '#if IS_ENABLED(CONFIG_ULTRAWIDELOCK_UNLOCK_GATE_SWITCH)\n#include "unlock_gate.h"\n#endif\n')

# Gate the two passive approach unlock actions immediately before the existing
# grant side effects. Intentional Home/NFC commands do not traverse this switch.
main = MAIN.read_text()
if 'passive unlock withheld by owner gate' not in main:
    needle = '''\t\tcase ULTRAWIDELOCK_APPROACH_UNLOCK_PREDICT:\n\t\tcase ULTRAWIDELOCK_APPROACH_UNLOCK_THRESHOLD:\n'''
    if needle not in main:
        raise SystemExit('passive unlock case anchor not found in main.c')
    gate = needle + '''#if IS_ENABLED(CONFIG_ULTRAWIDELOCK_UNLOCK_GATE_SWITCH)\n\t\t\tif (!unlock_gate_allows_passive()) {\n\t\t\t\tultrawidelock_approach_veto(&approach);\n\t\t\t\tLOG_INF("passive unlock withheld by owner gate");\n\t\t\t\tbreak;\n\t\t\t}\n#endif\n'''
    main = main.replace(needle, gate, 1)
    MAIN.write_text(main)

# Matter public definitions/state for endpoint 3.
h = HDR.read_text()
if 'MATTER_CLUSTER_ON_OFF' not in h:
    h = h.replace('#define MATTER_CLUSTER_DOOR_LOCK               0x0101u\n',
                  '#define MATTER_CLUSTER_DOOR_LOCK               0x0101u\n'
                  '#define MATTER_CLUSTER_ON_OFF                  0x0006u\n\n'
                  '#define MATTER_ATTR_ON_OFF_ON_OFF              0x0000u\n'
                  '#define MATTER_CMD_ON_OFF_OFF                  0x0000u\n'
                  '#define MATTER_CMD_ON_OFF_ON                   0x0001u\n'
                  '#define MATTER_CMD_ON_OFF_TOGGLE               0x0002u\n', 1)
if 'MATTER_DEVICE_TYPE_ON_OFF_PLUGIN_UNIT' not in h:
    h = h.replace('#define MATTER_DEVICE_TYPE_LOCK_REV  3u\n',
                  '#define MATTER_DEVICE_TYPE_LOCK_REV  3u\n\n'
                  '#define MATTER_DEVICE_TYPE_ON_OFF_PLUGIN_UNIT 0x010Au\n'
                  '#define MATTER_DEVICE_TYPE_ON_OFF_PLUGIN_REV  1u\n', 1)
if 'MATTER_ENDPOINT_UNLOCK_GATE' not in h:
    h = h.replace('#define MATTER_ENDPOINT_LOCK 1u\n',
                  '#define MATTER_ENDPOINT_LOCK 1u\n'
                  '#define MATTER_ENDPOINT_UNLOCK_GATE 3u\n', 1)
HDR.write_text(h)

# Matter model. Keep runtime state in unlock_gate.c rather than matter_device_info,
# so the feature remains isolated and the general device-info struct stays clean.
c = CLU.read_text()
if '#include "unlock_gate.h"' not in c:
    c = c.replace('#include "matter_clusters.h"\n',
                  '#include "matter_clusters.h"\n#if IS_ENABLED(CONFIG_ULTRAWIDELOCK_UNLOCK_GATE_SWITCH)\n#include "unlock_gate.h"\n#endif\n', 1)

if 'k_unlock_gate_servers' not in c:
    anchor = '''static const uint32_t k_lock_servers[] = {\n\tMATTER_CLUSTER_DESCRIPTOR,\n\tMATTER_CLUSTER_DOOR_LOCK,\n\tMATTER_CLUSTER_APPROACH_DIRECTION,\n};\n'''
    addition = anchor + '''\n#if IS_ENABLED(CONFIG_ULTRAWIDELOCK_UNLOCK_GATE_SWITCH)\nstatic const uint32_t k_unlock_gate_servers[] = {\n\tMATTER_CLUSTER_DESCRIPTOR,\n\tMATTER_CLUSTER_ON_OFF,\n};\n#endif\n'''
    if anchor not in c:
        raise SystemExit('k_lock_servers anchor not found')
    c = c.replace(anchor, addition, 1)

# has_cluster endpoint 3.
if 'endpoint == MATTER_ENDPOINT_UNLOCK_GATE' not in c.split('static bool has_cluster',1)[1].split('static const uint16_t k_endpoints',1)[0]:
    anchor = '''\tif (endpoint == MATTER_ENDPOINT_LOCK) {\n\t\treturn cluster == MATTER_CLUSTER_DESCRIPTOR ||\n\t\t       cluster == MATTER_CLUSTER_DOOR_LOCK ||\n\t\t       cluster == MATTER_CLUSTER_APPROACH_DIRECTION;\n\t}\n'''
    addition = anchor + '''#if IS_ENABLED(CONFIG_ULTRAWIDELOCK_UNLOCK_GATE_SWITCH)\n\tif (endpoint == MATTER_ENDPOINT_UNLOCK_GATE) {\n\t\treturn cluster == MATTER_CLUSTER_DESCRIPTOR || cluster == MATTER_CLUSTER_ON_OFF;\n\t}\n#endif\n'''
    if anchor not in c:
        raise SystemExit('has_cluster lock anchor not found')
    c = c.replace(anchor, addition, 1)

# endpoints list; compile-time optional, independent of any battery endpoint 2.
if 'MATTER_ENDPOINT_UNLOCK_GATE,' not in c:
    anchor = '''static const uint16_t k_endpoints[] = {\n\tMATTER_ENDPOINT_ROOT,\n\tMATTER_ENDPOINT_LOCK,\n};'''
    repl = '''static const uint16_t k_endpoints[] = {\n\tMATTER_ENDPOINT_ROOT,\n\tMATTER_ENDPOINT_LOCK,\n#if IS_ENABLED(CONFIG_ULTRAWIDELOCK_UNLOCK_GATE_SWITCH)\n\tMATTER_ENDPOINT_UNLOCK_GATE,\n#endif\n};'''
    if anchor not in c:
        raise SystemExit('k_endpoints anchor not found')
    c = c.replace(anchor, repl, 1)

# Attribute status for the endpoint, including standard globals.
status_marker = '''\t/*\n\t * Endpoint, then cluster, then attribute. The ORDER is the answer:\n'''
if 'cluster == MATTER_CLUSTER_ON_OFF' not in c.split('static uint8_t attr_status',1)[1].split('if (endpoint == MATTER_ENDPOINT_LOCK)',1)[0]:
    block = '''#if IS_ENABLED(CONFIG_ULTRAWIDELOCK_UNLOCK_GATE_SWITCH)\n\tif (endpoint == MATTER_ENDPOINT_UNLOCK_GATE) {\n\t\tif (cluster == MATTER_CLUSTER_DESCRIPTOR) {\n\t\t\tswitch (attribute) {\n\t\t\tcase MATTER_ATTR_DESC_DEVICE_TYPE_LIST:\n\t\t\tcase MATTER_ATTR_DESC_SERVER_LIST:\n\t\t\tcase MATTER_ATTR_DESC_CLIENT_LIST:\n\t\t\tcase MATTER_ATTR_DESC_PARTS_LIST:\n\t\t\t\treturn MATTER_IM_STATUS_SUCCESS;\n\t\t\tdefault:\n\t\t\t\treturn MATTER_IM_STATUS_UNSUPPORTED_ATTRIBUTE;\n\t\t\t}\n\t\t}\n\t\tif (cluster != MATTER_CLUSTER_ON_OFF) return MATTER_IM_STATUS_UNSUPPORTED_CLUSTER;\n\t\tswitch (attribute) {\n\t\tcase MATTER_ATTR_ON_OFF_ON_OFF:\n\t\tcase MATTER_ATTR_FEATURE_MAP:\n\t\tcase MATTER_ATTR_CLUSTER_REVISION:\n\t\tcase MATTER_ATTR_ATTRIBUTE_LIST:\n\t\tcase MATTER_ATTR_ACCEPTED_CMD_LIST:\n\t\tcase MATTER_ATTR_GENERATED_CMD_LIST:\n\t\t\treturn MATTER_IM_STATUS_SUCCESS;\n\t\tdefault:\n\t\t\treturn MATTER_IM_STATUS_UNSUPPORTED_ATTRIBUTE;\n\t\t}\n\t}\n#endif\n\n'''
    if status_marker not in c:
        raise SystemExit('attr_status marker not found')
    c = c.replace(status_marker, block + status_marker, 1)

# Encoder for endpoint 3. Use standard global metadata so controllers treat it
# as a normal On/Off endpoint rather than an incomplete custom cluster.
if 'static void unlock_gate_attr_value' not in c:
    fn = r'''
#if IS_ENABLED(CONFIG_ULTRAWIDELOCK_UNLOCK_GATE_SWITCH)
static const uint32_t k_unlock_gate_attrs[] = {
	MATTER_ATTR_ON_OFF_ON_OFF,
	MATTER_ATTR_GENERATED_CMD_LIST,
	MATTER_ATTR_ACCEPTED_CMD_LIST,
	MATTER_ATTR_ATTRIBUTE_LIST,
	MATTER_ATTR_FEATURE_MAP,
	MATTER_ATTR_CLUSTER_REVISION,
};

static void unlock_gate_attr_value(uint32_t cluster, uint32_t attribute,
				   struct matter_tlv_writer *w, matter_tlv_tag_t tag)
{
	size_t i;

	if (cluster == MATTER_CLUSTER_DESCRIPTOR) {
		switch (attribute) {
		case MATTER_ATTR_DESC_DEVICE_TYPE_LIST:
			(void)matter_tlv_start_container(w, tag, MATTER_TLV_ARRAY);
			(void)matter_tlv_start_container(w, MATTER_TLV_ANON, MATTER_TLV_STRUCTURE);
			(void)matter_tlv_put_u64(w, MATTER_TLV_CTX(TAG_DEVTYPE_TYPE),
						 MATTER_DEVICE_TYPE_ON_OFF_PLUGIN_UNIT);
			(void)matter_tlv_put_u64(w, MATTER_TLV_CTX(TAG_DEVTYPE_REVISION),
						 MATTER_DEVICE_TYPE_ON_OFF_PLUGIN_REV);
			(void)matter_tlv_end_container(w);
			(void)matter_tlv_end_container(w);
			return;
		case MATTER_ATTR_DESC_SERVER_LIST:
			(void)matter_tlv_start_container(w, tag, MATTER_TLV_ARRAY);
			for (i = 0; i < sizeof(k_unlock_gate_servers) / sizeof(k_unlock_gate_servers[0]); i++)
				(void)matter_tlv_put_u64(w, MATTER_TLV_ANON, k_unlock_gate_servers[i]);
			(void)matter_tlv_end_container(w);
			return;
		case MATTER_ATTR_DESC_CLIENT_LIST:
		case MATTER_ATTR_DESC_PARTS_LIST:
			(void)matter_tlv_start_container(w, tag, MATTER_TLV_ARRAY);
			(void)matter_tlv_end_container(w);
			return;
		default:
			return;
		}
	}

	if (cluster != MATTER_CLUSTER_ON_OFF) return;
	switch (attribute) {
	case MATTER_ATTR_ON_OFF_ON_OFF:
		(void)matter_tlv_put_bool(w, tag, unlock_gate_allows_passive()); return;
	case MATTER_ATTR_FEATURE_MAP:
		(void)matter_tlv_put_u64(w, tag, 0u); return;
	case MATTER_ATTR_CLUSTER_REVISION:
		(void)matter_tlv_put_u64(w, tag, 6u); return;
	case MATTER_ATTR_ATTRIBUTE_LIST:
		(void)matter_tlv_start_container(w, tag, MATTER_TLV_ARRAY);
		for (i = 0; i < sizeof(k_unlock_gate_attrs) / sizeof(k_unlock_gate_attrs[0]); i++)
			(void)matter_tlv_put_u64(w, MATTER_TLV_ANON, k_unlock_gate_attrs[i]);
		(void)matter_tlv_end_container(w); return;
	case MATTER_ATTR_ACCEPTED_CMD_LIST:
		(void)matter_tlv_start_container(w, tag, MATTER_TLV_ARRAY);
		(void)matter_tlv_put_u64(w, MATTER_TLV_ANON, MATTER_CMD_ON_OFF_OFF);
		(void)matter_tlv_put_u64(w, MATTER_TLV_ANON, MATTER_CMD_ON_OFF_ON);
		(void)matter_tlv_put_u64(w, MATTER_TLV_ANON, MATTER_CMD_ON_OFF_TOGGLE);
		(void)matter_tlv_end_container(w); return;
	case MATTER_ATTR_GENERATED_CMD_LIST:
		(void)matter_tlv_start_container(w, tag, MATTER_TLV_ARRAY);
		(void)matter_tlv_end_container(w); return;
	default:
		return;
	}
}
#endif

'''
    anchor = 'static void lock_attr_value('
    if anchor not in c:
        raise SystemExit('lock_attr_value anchor not found')
    c = c.replace(anchor, fn + anchor, 1)

# Route endpoint value dispatch.
if 'unlock_gate_attr_value(cluster, attribute, w, tag)' not in c:
    anchor = '''\tif (endpoint == MATTER_ENDPOINT_LOCK) {\n\t\tlock_attr_value(info, cluster, attribute, w, tag);\n\t\treturn;\n\t}\n'''
    repl = anchor + '''#if IS_ENABLED(CONFIG_ULTRAWIDELOCK_UNLOCK_GATE_SWITCH)\n\tif (endpoint == MATTER_ENDPOINT_UNLOCK_GATE) {\n\t\tunlock_gate_attr_value(cluster, attribute, w, tag);\n\t\treturn;\n\t}\n#endif\n'''
    if anchor not in c:
        raise SystemExit('attr_value route anchor not found')
    c = c.replace(anchor, repl, 1)

# Root PartsList includes endpoint 3 only when enabled.
if 'MATTER_ENDPOINT_UNLOCK_GATE' not in c.split('case MATTER_ATTR_DESC_PARTS_LIST:',1)[1].split('default:',1)[0]:
    anchor = '''\t\t\t(void)matter_tlv_put_u64(w, MATTER_TLV_ANON, MATTER_ENDPOINT_LOCK);\n'''
    repl = anchor + '''#if IS_ENABLED(CONFIG_ULTRAWIDELOCK_UNLOCK_GATE_SWITCH)\n\t\t\t(void)matter_tlv_put_u64(w, MATTER_TLV_ANON, MATTER_ENDPOINT_UNLOCK_GATE);\n#endif\n'''
    if anchor not in c:
        raise SystemExit('root PartsList anchor not found')
    c = c.replace(anchor, repl, 1)

# list_clusters/list_attrs support wildcard discovery.
if '*out = k_unlock_gate_servers' not in c:
    anchor = '''\tif (endpoint == MATTER_ENDPOINT_LOCK) {\n\t\t*out = k_lock_servers;\n\t\treturn sizeof(k_lock_servers) / sizeof(k_lock_servers[0]);\n\t}\n'''
    repl = anchor + '''#if IS_ENABLED(CONFIG_ULTRAWIDELOCK_UNLOCK_GATE_SWITCH)\n\tif (endpoint == MATTER_ENDPOINT_UNLOCK_GATE) {\n\t\t*out = k_unlock_gate_servers;\n\t\treturn sizeof(k_unlock_gate_servers) / sizeof(k_unlock_gate_servers[0]);\n\t}\n#endif\n'''
    if anchor not in c:
        raise SystemExit('list_clusters anchor not found')
    c = c.replace(anchor, repl, 1)

la = 'static size_t list_attrs(void *ctx, uint16_t endpoint, uint32_t cluster, const uint32_t **out)\n{\n\t(void)ctx;\n'
if 'endpoint == MATTER_ENDPOINT_UNLOCK_GATE' not in c.split(la,1)[1].split('if (endpoint == MATTER_ENDPOINT_LOCK)',1)[0]:
    block = '''#if IS_ENABLED(CONFIG_ULTRAWIDELOCK_UNLOCK_GATE_SWITCH)\n\tif (endpoint == MATTER_ENDPOINT_UNLOCK_GATE) {\n\t\tif (cluster == MATTER_CLUSTER_DESCRIPTOR) { *out = k_desc_attrs; return sizeof(k_desc_attrs) / sizeof(k_desc_attrs[0]); }\n\t\tif (cluster == MATTER_CLUSTER_ON_OFF) { *out = k_unlock_gate_attrs; return sizeof(k_unlock_gate_attrs) / sizeof(k_unlock_gate_attrs[0]); }\n\t\treturn 0u;\n\t}\n#endif\n'''
    if la not in c:
        raise SystemExit('list_attrs anchor not found')
    c = c.replace(la, la + block, 1)

# On/Off commands mutate only the isolated runtime gate.
cmd = 'static uint8_t command(void *ctx, const struct matter_im_invoke *inv, uint32_t *response_command)\n{\n'
if 'inv->endpoint == MATTER_ENDPOINT_UNLOCK_GATE' not in c:
    block = '''#if IS_ENABLED(CONFIG_ULTRAWIDELOCK_UNLOCK_GATE_SWITCH)\n\tif (inv->endpoint == MATTER_ENDPOINT_UNLOCK_GATE) {\n\t\tif (inv->cluster != MATTER_CLUSTER_ON_OFF) return MATTER_IM_STATUS_UNSUPPORTED_CLUSTER;\n\t\t*response_command = MATTER_IM_NO_RESPONSE;\n\t\tswitch (inv->command) {\n\t\tcase MATTER_CMD_ON_OFF_OFF: unlock_gate_set(false); return MATTER_IM_STATUS_SUCCESS;\n\t\tcase MATTER_CMD_ON_OFF_ON: unlock_gate_set(true); return MATTER_IM_STATUS_SUCCESS;\n\t\tcase MATTER_CMD_ON_OFF_TOGGLE: unlock_gate_toggle(); return MATTER_IM_STATUS_SUCCESS;\n\t\tdefault: return MATTER_IM_STATUS_UNSUPPORTED_COMMAND;\n\t\t}\n\t}\n#endif\n'''
    if cmd not in c:
        raise SystemExit('command anchor not found')
    c = c.replace(cmd, cmd + block, 1)

CLU.write_text(c)
print('Applied standalone unlock-gate switch feature (endpoint 3, off by default).')
