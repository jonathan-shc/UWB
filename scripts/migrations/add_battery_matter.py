#!/usr/bin/env python3
"""Add Matter Power Source presentation for the standalone battery feature.

Run after add_battery_status.py. This mirrors the earlier hardware-validated
shape: a dedicated Power Source utility endpoint referenced by the root.
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
        raise SystemExit(f"anchor not found in {path}: {old[:120]!r}")
    path.write_text(text.replace(old, new, 1))


replace_once(HDR, '#define MATTER_CLUSTER_DOOR_LOCK               0x0101u\n',
'''#define MATTER_CLUSTER_DOOR_LOCK               0x0101u
#ifdef CONFIG_CUSTOM_BATTERY_STATUS
#define MATTER_CLUSTER_POWER_SOURCE            0x002Fu
#define MATTER_ATTR_PS_STATUS                   0x0000u
#define MATTER_ATTR_PS_ORDER                    0x0001u
#define MATTER_ATTR_PS_DESCRIPTION              0x0002u
#define MATTER_ATTR_PS_BAT_PERCENT_REMAINING    0x000Cu
#define MATTER_ATTR_PS_BAT_CHARGE_LEVEL         0x000Eu
#define MATTER_ATTR_PS_BAT_REPLACEMENT_NEEDED  0x000Fu
#define MATTER_ATTR_PS_BAT_REPLACEABILITY       0x0010u
#define MATTER_ATTR_PS_BAT_PRESENT              0x0011u
#define MATTER_ATTR_PS_ENDPOINT_LIST            0x001Fu
#define MATTER_PS_STATUS_ACTIVE                 1u
#define MATTER_PS_FEATURE_BATTERY               0x02u
#define MATTER_PS_FEATURE_RECHARGEABLE          0x04u
#define MATTER_PS_REPLACEABILITY_USER           2u
#endif
''')
replace_once(HDR, '#define MATTER_DEVICE_TYPE_LOCK_REV  3u\n',
'''#define MATTER_DEVICE_TYPE_LOCK_REV  3u
#ifdef CONFIG_CUSTOM_BATTERY_STATUS
#define MATTER_DEVICE_TYPE_POWER_SOURCE     0x0011u
#define MATTER_DEVICE_TYPE_POWER_SOURCE_REV 1u
#endif
''')
replace_once(HDR, '#define MATTER_ENDPOINT_LOCK 1u\n',
'''#define MATTER_ENDPOINT_LOCK 1u
#ifdef CONFIG_CUSTOM_BATTERY_STATUS
#define MATTER_ENDPOINT_POWER_SOURCE 2u
#endif
''')
replace_once(HDR, '\tuint8_t lock_state;\n',
'''\tuint8_t lock_state;
#ifdef CONFIG_CUSTOM_BATTERY_STATUS
\tuint8_t battery_percent;
#endif
''')

replace_once(CLUSTERS,
'''static const uint32_t k_lock_servers[] = {
\tMATTER_CLUSTER_DESCRIPTOR,
\tMATTER_CLUSTER_DOOR_LOCK,
\tMATTER_CLUSTER_APPROACH_DIRECTION,
};
''',
'''static const uint32_t k_lock_servers[] = {
\tMATTER_CLUSTER_DESCRIPTOR,
\tMATTER_CLUSTER_DOOR_LOCK,
\tMATTER_CLUSTER_APPROACH_DIRECTION,
};
#ifdef CONFIG_CUSTOM_BATTERY_STATUS
static const uint32_t k_power_servers[] = {
\tMATTER_CLUSTER_DESCRIPTOR,
\tMATTER_CLUSTER_POWER_SOURCE,
};
#endif
''')

# Scope each insertion by its function signature. The previous migration used
# generic "if (endpoint == LOCK)" anchors; those also occur in has_cluster,
# attr_status and list_attrs, causing code to land in the wrong function.
has_sig = 'static bool has_cluster(void *ctx, uint16_t endpoint, uint32_t cluster)\n{\n'
has_old = has_sig + '''\t(void)ctx;

\tif (endpoint == MATTER_ENDPOINT_LOCK) {
'''
has_new = has_sig + '''\t(void)ctx;

#ifdef CONFIG_CUSTOM_BATTERY_STATUS
\tif (endpoint == MATTER_ENDPOINT_POWER_SOURCE) {
\t\treturn cluster == MATTER_CLUSTER_DESCRIPTOR || cluster == MATTER_CLUSTER_POWER_SOURCE;
\t}
#endif
\tif (endpoint == MATTER_ENDPOINT_LOCK) {
'''
replace_once(CLUSTERS, has_old, has_new)

replace_once(CLUSTERS,
'''static const uint16_t k_endpoints[] = {
\tMATTER_ENDPOINT_ROOT,
\tMATTER_ENDPOINT_LOCK,
};
''',
'''static const uint16_t k_endpoints[] = {
\tMATTER_ENDPOINT_ROOT,
\tMATTER_ENDPOINT_LOCK,
#ifdef CONFIG_CUSTOM_BATTERY_STATUS
\tMATTER_ENDPOINT_POWER_SOURCE,
#endif
};
''')

attr_sig = 'static uint8_t attr_status(void *ctx, uint16_t endpoint, uint32_t cluster, uint32_t attribute)\n{\n'
attr_old = attr_sig + '''\t(void)ctx;

\tif (endpoint == MATTER_ENDPOINT_LOCK) {
'''
attr_new = attr_sig + '''\t(void)ctx;

#ifdef CONFIG_CUSTOM_BATTERY_STATUS
\tif (endpoint == MATTER_ENDPOINT_POWER_SOURCE) {
\t\tif (cluster == MATTER_CLUSTER_DESCRIPTOR) {
\t\t\tswitch (attribute) {
\t\t\tcase MATTER_ATTR_DESC_DEVICE_TYPE_LIST:
\t\t\tcase MATTER_ATTR_DESC_SERVER_LIST:
\t\t\tcase MATTER_ATTR_DESC_CLIENT_LIST:
\t\t\tcase MATTER_ATTR_DESC_PARTS_LIST:
\t\t\t\treturn MATTER_IM_STATUS_SUCCESS;
\t\t\tdefault:
\t\t\t\treturn MATTER_IM_STATUS_UNSUPPORTED_ATTRIBUTE;
\t\t\t}
\t\t}
\t\tif (cluster != MATTER_CLUSTER_POWER_SOURCE) return MATTER_IM_STATUS_UNSUPPORTED_CLUSTER;
\t\tswitch (attribute) {
\t\tcase MATTER_ATTR_PS_STATUS:
\t\tcase MATTER_ATTR_PS_ORDER:
\t\tcase MATTER_ATTR_PS_DESCRIPTION:
\t\tcase MATTER_ATTR_PS_BAT_PERCENT_REMAINING:
\t\tcase MATTER_ATTR_PS_BAT_CHARGE_LEVEL:
\t\tcase MATTER_ATTR_PS_BAT_REPLACEMENT_NEEDED:
\t\tcase MATTER_ATTR_PS_BAT_REPLACEABILITY:
\t\tcase MATTER_ATTR_PS_BAT_PRESENT:
\t\tcase MATTER_ATTR_PS_ENDPOINT_LIST:
\t\tcase MATTER_ATTR_FEATURE_MAP:
\t\t\treturn MATTER_IM_STATUS_SUCCESS;
\t\tdefault:
\t\t\treturn MATTER_IM_STATUS_UNSUPPORTED_ATTRIBUTE;
\t\t}
\t}
#endif
\tif (endpoint == MATTER_ENDPOINT_LOCK) {
'''
replace_once(CLUSTERS, attr_old, attr_new)

# Value encoder can be declared here because the needed server table and tags
# are already defined, while attribute arrays may remain later in the file.
lock_value_sig = '''static void lock_attr_value(const struct matter_device_info *info, uint32_t cluster,
\t\t\t    uint32_t attribute, struct matter_tlv_writer *w, matter_tlv_tag_t tag)
'''
power_value = '''#ifdef CONFIG_CUSTOM_BATTERY_STATUS
static void power_attr_value(const struct matter_device_info *info, uint32_t cluster,
                             uint32_t attribute, struct matter_tlv_writer *w, matter_tlv_tag_t tag)
{
\tsize_t i;
\tif (cluster == MATTER_CLUSTER_DESCRIPTOR) {
\t\tswitch (attribute) {
\t\tcase MATTER_ATTR_DESC_DEVICE_TYPE_LIST:
\t\t\t(void)matter_tlv_start_container(w, tag, MATTER_TLV_ARRAY);
\t\t\t(void)matter_tlv_start_container(w, MATTER_TLV_ANON, MATTER_TLV_STRUCTURE);
\t\t\t(void)matter_tlv_put_u64(w, MATTER_TLV_CTX(TAG_DEVTYPE_TYPE), MATTER_DEVICE_TYPE_POWER_SOURCE);
\t\t\t(void)matter_tlv_put_u64(w, MATTER_TLV_CTX(TAG_DEVTYPE_REVISION), MATTER_DEVICE_TYPE_POWER_SOURCE_REV);
\t\t\t(void)matter_tlv_end_container(w); (void)matter_tlv_end_container(w); return;
\t\tcase MATTER_ATTR_DESC_SERVER_LIST:
\t\t\t(void)matter_tlv_start_container(w, tag, MATTER_TLV_ARRAY);
\t\t\tfor (i = 0; i < sizeof(k_power_servers)/sizeof(k_power_servers[0]); i++)
\t\t\t\t(void)matter_tlv_put_u64(w, MATTER_TLV_ANON, k_power_servers[i]);
\t\t\t(void)matter_tlv_end_container(w); return;
\t\tcase MATTER_ATTR_DESC_CLIENT_LIST:
\t\tcase MATTER_ATTR_DESC_PARTS_LIST:
\t\t\t(void)matter_tlv_start_container(w, tag, MATTER_TLV_ARRAY); (void)matter_tlv_end_container(w); return;
\t\tdefault: return;
\t\t}
\t}
\tif (cluster != MATTER_CLUSTER_POWER_SOURCE) return;
\tswitch (attribute) {
\tcase MATTER_ATTR_PS_STATUS: (void)matter_tlv_put_u64(w, tag, MATTER_PS_STATUS_ACTIVE); return;
\tcase MATTER_ATTR_PS_ORDER: (void)matter_tlv_put_u64(w, tag, 0u); return;
\tcase MATTER_ATTR_PS_DESCRIPTION: (void)matter_tlv_put_utf8(w, tag, "Battery", 7u); return;
\tcase MATTER_ATTR_PS_BAT_PERCENT_REMAINING: (void)matter_tlv_put_u64(w, tag, (uint64_t)info->battery_percent * 2u); return;
\tcase MATTER_ATTR_PS_BAT_CHARGE_LEVEL: (void)matter_tlv_put_u64(w, tag, info->battery_percent <= 10u ? 2u : info->battery_percent <= 20u ? 1u : 0u); return;
\tcase MATTER_ATTR_PS_BAT_REPLACEMENT_NEEDED: (void)matter_tlv_put_bool(w, tag, false); return;
\tcase MATTER_ATTR_PS_BAT_REPLACEABILITY: (void)matter_tlv_put_u64(w, tag, MATTER_PS_REPLACEABILITY_USER); return;
\tcase MATTER_ATTR_PS_BAT_PRESENT: (void)matter_tlv_put_bool(w, tag, true); return;
\tcase MATTER_ATTR_PS_ENDPOINT_LIST:
\t\t(void)matter_tlv_start_container(w, tag, MATTER_TLV_ARRAY); (void)matter_tlv_put_u64(w, MATTER_TLV_ANON, MATTER_ENDPOINT_LOCK); (void)matter_tlv_end_container(w); return;
\tcase MATTER_ATTR_FEATURE_MAP: (void)matter_tlv_put_u64(w, tag, MATTER_PS_FEATURE_BATTERY | MATTER_PS_FEATURE_RECHARGEABLE); return;
\tdefault: return;
\t}
}
#endif

''' + lock_value_sig
replace_once(CLUSTERS, lock_value_sig, power_value)

# Dispatch value by endpoint, scoped to attr_value.
value_sig = 'static void attr_value(void *ctx, uint16_t endpoint, uint32_t cluster, uint32_t attribute,\n'
text = CLUSTERS.read_text()
pos = text.find(value_sig)
if pos < 0: raise SystemExit('attr_value signature not found')
lockpos = text.find('\tif (endpoint == MATTER_ENDPOINT_LOCK) {\n', pos)
if lockpos < 0: raise SystemExit('attr_value lock dispatch not found')
insert = '''#ifdef CONFIG_CUSTOM_BATTERY_STATUS
\tif (endpoint == MATTER_ENDPOINT_POWER_SOURCE) {
\t\tpower_attr_value(info, cluster, attribute, w, tag);
\t\treturn;
\t}
#endif
'''
if insert not in text[pos:lockpos+len(insert)+200]:
    text = text[:lockpos] + insert + text[lockpos:]
    CLUSTERS.write_text(text)

# Root PartsList: scope search after root Descriptor branch comment/string.
text = CLUSTERS.read_text()
needle = '\t\t\t(void)matter_tlv_put_u64(w, MATTER_TLV_ANON, MATTER_ENDPOINT_LOCK);\n\t\t\t(void)matter_tlv_end_container(w);\n'
idx = text.find(needle)
if idx < 0: raise SystemExit('root PartsList anchor not found')
replacement = '''\t\t\t(void)matter_tlv_put_u64(w, MATTER_TLV_ANON, MATTER_ENDPOINT_LOCK);
#ifdef CONFIG_CUSTOM_BATTERY_STATUS
\t\t\t(void)matter_tlv_put_u64(w, MATTER_TLV_ANON, MATTER_ENDPOINT_POWER_SOURCE);
#endif
\t\t\t(void)matter_tlv_end_container(w);
'''
text = text[:idx] + replacement + text[idx+len(needle):]
CLUSTERS.write_text(text)

# Arrays are declared before list_attrs uses them.
replace_once(CLUSTERS, 'static const uint32_t k_desc_attrs[] = {\n',
'''#ifdef CONFIG_CUSTOM_BATTERY_STATUS
static const uint32_t k_power_attrs[] = {
\tMATTER_ATTR_PS_STATUS, MATTER_ATTR_PS_ORDER, MATTER_ATTR_PS_DESCRIPTION,
\tMATTER_ATTR_PS_BAT_PERCENT_REMAINING, MATTER_ATTR_PS_BAT_CHARGE_LEVEL,
\tMATTER_ATTR_PS_BAT_REPLACEMENT_NEEDED, MATTER_ATTR_PS_BAT_REPLACEABILITY,
\tMATTER_ATTR_PS_BAT_PRESENT, MATTER_ATTR_PS_ENDPOINT_LIST, MATTER_ATTR_FEATURE_MAP,
};
#endif

static const uint32_t k_desc_attrs[] = {
''')

# list_servers scoped by signature.
ls_sig = 'static size_t list_servers(void *ctx, uint16_t endpoint, const uint32_t **out)\n{\n'
ls_old = ls_sig + '''\t(void)ctx;

\tif (endpoint == MATTER_ENDPOINT_LOCK) {
'''
ls_new = ls_sig + '''\t(void)ctx;

#ifdef CONFIG_CUSTOM_BATTERY_STATUS
\tif (endpoint == MATTER_ENDPOINT_POWER_SOURCE) {
\t\t*out = k_power_servers;
\t\treturn sizeof(k_power_servers) / sizeof(k_power_servers[0]);
\t}
#endif
\tif (endpoint == MATTER_ENDPOINT_LOCK) {
'''
replace_once(CLUSTERS, ls_old, ls_new)

# list_attrs scoped by signature; arrays are now already defined.
la_sig = 'static size_t list_attrs(void *ctx, uint16_t endpoint, uint32_t cluster, const uint32_t **out)\n{\n'
la_old = la_sig + '''\t(void)ctx;

\tif (endpoint == MATTER_ENDPOINT_LOCK) {
'''
la_new = la_sig + '''\t(void)ctx;

#ifdef CONFIG_CUSTOM_BATTERY_STATUS
\tif (endpoint == MATTER_ENDPOINT_POWER_SOURCE) {
\t\tif (cluster == MATTER_CLUSTER_DESCRIPTOR) {
\t\t\t*out = k_desc_attrs;
\t\t\treturn sizeof(k_desc_attrs) / sizeof(k_desc_attrs[0]);
\t\t}
\t\tif (cluster == MATTER_CLUSTER_POWER_SOURCE) {
\t\t\t*out = k_power_attrs;
\t\t\treturn sizeof(k_power_attrs) / sizeof(k_power_attrs[0]);
\t\t}
\t\treturn 0u;
\t}
#endif
\tif (endpoint == MATTER_ENDPOINT_LOCK) {
'''
replace_once(CLUSTERS, la_old, la_new)

replace_once(COMMISSION, '#include "status_led.h" /* the lock LED; a tile tap has to move it too */\n',
'''#include "status_led.h" /* the lock LED; a tile tap has to move it too */
#ifdef CONFIG_CUSTOM_BATTERY_STATUS
#include "battery_status.h"
#endif
''')
replace_once(COMMISSION, 'int matter_commission_init(void)\n{\n',
'''int matter_commission_init(void)
{
#ifdef CONFIG_CUSTOM_BATTERY_STATUS
\ts_info.battery_percent = battery_status_percent();
#endif
''')

print("Applied dedicated Matter Power Source endpoint integration.")
