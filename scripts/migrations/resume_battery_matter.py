#!/usr/bin/env python3
"""Resume/repair a partially applied battery Matter migration.

This script is intentionally written for the state left behind when
add_battery_matter.py stops at its old attr_status anchor: the header,
k_power_servers, has_cluster and k_endpoints may already be patched while the
rest is still absent.  Every insertion below is idempotent and scoped by the
actual function body rather than fragile comments immediately after a
signature.

It also applies the Power Source conformance fix in the same pass: BAT only,
cluster revision 3, complete global attributes, and no unsupported RECHG bit.
"""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
APP = ROOT / "apps" / "dwm3001cdk-lock"
SRC = APP / "src"
HDR = ROOT / "modules" / "ultrawidelock_matter" / "include" / "matter_clusters.h"
CLUSTERS = ROOT / "modules" / "ultrawidelock_matter" / "src" / "matter_clusters.c"
COMMISSION = SRC / "matter_commission.c"


def replace_once(path: Path, old: str, new: str) -> None:
    text = path.read_text()
    if new in text:
        return
    if old not in text:
        raise SystemExit(f"anchor not found in {path}: {old[:160]!r}")
    path.write_text(text.replace(old, new, 1))


def insert_before_in_function(path: Path, signature: str, needle: str, block: str) -> None:
    text = path.read_text()
    pos = text.find(signature)
    if pos < 0:
        raise SystemExit(f"function signature not found in {path}: {signature!r}")
    if block in text[pos:]:
        return
    at = text.find(needle, pos)
    if at < 0:
        raise SystemExit(f"function-local anchor not found in {path}: {needle!r}")
    path.write_text(text[:at] + block + text[at:])


# The first migration may already have installed these old definitions. Convert
# them to a deliberately minimal, coherent BAT-only Power Source.
text = HDR.read_text()
if "MATTER_PS_FEATURE_RECHARGEABLE" in text:
    text = text.replace(
        "#define MATTER_PS_FEATURE_BATTERY               0x02u\n"
        "#define MATTER_PS_FEATURE_RECHARGEABLE          0x04u\n"
        "#define MATTER_PS_REPLACEABILITY_USER           2u\n",
        "#define MATTER_PS_FEATURE_BATTERY               0x02u\n"
        "#define MATTER_PS_CLUSTER_REVISION              3u\n"
        "#define MATTER_PS_REPLACEABILITY_NOT_REPLACEABLE 1u\n",
        1,
    )
    HDR.write_text(text)
elif "MATTER_PS_CLUSTER_REVISION" not in text:
    raise SystemExit("Power Source definitions are missing; run add_battery_matter.py until its first failure first")


power_status = '''#ifdef CONFIG_CUSTOM_BATTERY_STATUS
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
\t\tif (cluster != MATTER_CLUSTER_POWER_SOURCE) {
\t\t\treturn MATTER_IM_STATUS_UNSUPPORTED_CLUSTER;
\t\t}
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
\t\tcase MATTER_ATTR_GENERATED_CMD_LIST:
\t\tcase MATTER_ATTR_ACCEPTED_CMD_LIST:
\t\tcase MATTER_ATTR_ATTRIBUTE_LIST:
\t\tcase MATTER_ATTR_FEATURE_MAP:
\t\tcase MATTER_ATTR_CLUSTER_REVISION:
\t\t\treturn MATTER_IM_STATUS_SUCCESS;
\t\tdefault:
\t\t\treturn MATTER_IM_STATUS_UNSUPPORTED_ATTRIBUTE;
\t\t}
\t}
#endif
'''
insert_before_in_function(
    CLUSTERS,
    "static uint8_t attr_status(void *ctx, uint16_t endpoint, uint32_t cluster, uint32_t attribute)\n{\n",
    "\tif (endpoint == MATTER_ENDPOINT_LOCK) {\n",
    power_status,
)


# Value encoder for endpoint 2.  AttributeList is emitted explicitly because
# k_power_attrs is declared later in this compact hand-written server.
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
\t\t\t(void)matter_tlv_end_container(w);
\t\t\t(void)matter_tlv_end_container(w);
\t\t\treturn;
\t\tcase MATTER_ATTR_DESC_SERVER_LIST:
\t\t\t(void)matter_tlv_start_container(w, tag, MATTER_TLV_ARRAY);
\t\t\tfor (i = 0u; i < sizeof(k_power_servers) / sizeof(k_power_servers[0]); i++) {
\t\t\t\t(void)matter_tlv_put_u64(w, MATTER_TLV_ANON, k_power_servers[i]);
\t\t\t}
\t\t\t(void)matter_tlv_end_container(w);
\t\t\treturn;
\t\tcase MATTER_ATTR_DESC_CLIENT_LIST:
\t\tcase MATTER_ATTR_DESC_PARTS_LIST:
\t\t\t(void)matter_tlv_start_container(w, tag, MATTER_TLV_ARRAY);
\t\t\t(void)matter_tlv_end_container(w);
\t\t\treturn;
\t\tdefault:
\t\t\treturn;
\t\t}
\t}

\tif (cluster != MATTER_CLUSTER_POWER_SOURCE) {
\t\treturn;
\t}

\tswitch (attribute) {
\tcase MATTER_ATTR_PS_STATUS:
\t\t(void)matter_tlv_put_u64(w, tag, MATTER_PS_STATUS_ACTIVE); return;
\tcase MATTER_ATTR_PS_ORDER:
\t\t(void)matter_tlv_put_u64(w, tag, 0u); return;
\tcase MATTER_ATTR_PS_DESCRIPTION:
\t\t(void)matter_tlv_put_utf8(w, tag, "Battery", 7u); return;
\tcase MATTER_ATTR_PS_BAT_PERCENT_REMAINING:
\t\t(void)matter_tlv_put_u64(w, tag, (uint64_t)info->battery_percent * 2u); return;
\tcase MATTER_ATTR_PS_BAT_CHARGE_LEVEL:
\t\t(void)matter_tlv_put_u64(w, tag,
\t\t\tinfo->battery_percent <= 10u ? 2u : info->battery_percent <= 20u ? 1u : 0u); return;
\tcase MATTER_ATTR_PS_BAT_REPLACEMENT_NEEDED:
\t\t(void)matter_tlv_put_bool(w, tag, false); return;
\tcase MATTER_ATTR_PS_BAT_REPLACEABILITY:
\t\t(void)matter_tlv_put_u64(w, tag, MATTER_PS_REPLACEABILITY_NOT_REPLACEABLE); return;
\tcase MATTER_ATTR_PS_BAT_PRESENT:
\t\t(void)matter_tlv_put_bool(w, tag, true); return;
\tcase MATTER_ATTR_PS_ENDPOINT_LIST:
\t\t(void)matter_tlv_start_container(w, tag, MATTER_TLV_ARRAY);
\t\t(void)matter_tlv_put_u64(w, MATTER_TLV_ANON, MATTER_ENDPOINT_LOCK);
\t\t(void)matter_tlv_end_container(w); return;
\tcase MATTER_ATTR_FEATURE_MAP:
\t\t(void)matter_tlv_put_u64(w, tag, MATTER_PS_FEATURE_BATTERY); return;
\tcase MATTER_ATTR_CLUSTER_REVISION:
\t\t(void)matter_tlv_put_u64(w, tag, MATTER_PS_CLUSTER_REVISION); return;
\tcase MATTER_ATTR_ATTRIBUTE_LIST:
\t\t(void)matter_tlv_start_container(w, tag, MATTER_TLV_ARRAY);
\t\t(void)matter_tlv_put_u64(w, MATTER_TLV_ANON, MATTER_ATTR_PS_STATUS);
\t\t(void)matter_tlv_put_u64(w, MATTER_TLV_ANON, MATTER_ATTR_PS_ORDER);
\t\t(void)matter_tlv_put_u64(w, MATTER_TLV_ANON, MATTER_ATTR_PS_DESCRIPTION);
\t\t(void)matter_tlv_put_u64(w, MATTER_TLV_ANON, MATTER_ATTR_PS_BAT_PERCENT_REMAINING);
\t\t(void)matter_tlv_put_u64(w, MATTER_TLV_ANON, MATTER_ATTR_PS_BAT_CHARGE_LEVEL);
\t\t(void)matter_tlv_put_u64(w, MATTER_TLV_ANON, MATTER_ATTR_PS_BAT_REPLACEMENT_NEEDED);
\t\t(void)matter_tlv_put_u64(w, MATTER_TLV_ANON, MATTER_ATTR_PS_BAT_REPLACEABILITY);
\t\t(void)matter_tlv_put_u64(w, MATTER_TLV_ANON, MATTER_ATTR_PS_BAT_PRESENT);
\t\t(void)matter_tlv_put_u64(w, MATTER_TLV_ANON, MATTER_ATTR_PS_ENDPOINT_LIST);
\t\t(void)matter_tlv_put_u64(w, MATTER_TLV_ANON, MATTER_ATTR_GENERATED_CMD_LIST);
\t\t(void)matter_tlv_put_u64(w, MATTER_TLV_ANON, MATTER_ATTR_ACCEPTED_CMD_LIST);
\t\t(void)matter_tlv_put_u64(w, MATTER_TLV_ANON, MATTER_ATTR_ATTRIBUTE_LIST);
\t\t(void)matter_tlv_put_u64(w, MATTER_TLV_ANON, MATTER_ATTR_FEATURE_MAP);
\t\t(void)matter_tlv_put_u64(w, MATTER_TLV_ANON, MATTER_ATTR_CLUSTER_REVISION);
\t\t(void)matter_tlv_end_container(w); return;
\tcase MATTER_ATTR_ACCEPTED_CMD_LIST:
\tcase MATTER_ATTR_GENERATED_CMD_LIST:
\t\t(void)matter_tlv_start_container(w, tag, MATTER_TLV_ARRAY);
\t\t(void)matter_tlv_end_container(w); return;
\tdefault:
\t\treturn;
\t}
}
#endif

'''
text = CLUSTERS.read_text()
if "static void power_attr_value(" not in text:
    pos = text.find(lock_value_sig)
    if pos < 0:
        raise SystemExit("lock_attr_value signature not found")
    CLUSTERS.write_text(text[:pos] + power_value + text[pos:])


power_dispatch = '''#ifdef CONFIG_CUSTOM_BATTERY_STATUS
\tif (endpoint == MATTER_ENDPOINT_POWER_SOURCE) {
\t\tpower_attr_value(info, cluster, attribute, w, tag);
\t\treturn;
\t}
#endif
'''
insert_before_in_function(
    CLUSTERS,
    "static void attr_value(void *ctx, uint16_t endpoint, uint32_t cluster, uint32_t attribute,\n",
    "\tif (endpoint == MATTER_ENDPOINT_LOCK) {\n",
    power_dispatch,
)


# Root Descriptor PartsList: append endpoint 2 immediately after endpoint 1.
text = CLUSTERS.read_text()
parts_line = "\t\t\t(void)matter_tlv_put_u64(w, MATTER_TLV_ANON, MATTER_ENDPOINT_LOCK);\n"
power_parts = "#ifdef CONFIG_CUSTOM_BATTERY_STATUS\n\t\t\t(void)matter_tlv_put_u64(w, MATTER_TLV_ANON, MATTER_ENDPOINT_POWER_SOURCE);\n#endif\n"
if power_parts not in text:
    pos = text.find(parts_line)
    if pos < 0:
        raise SystemExit("root PartsList lock endpoint line not found")
    pos += len(parts_line)
    CLUSTERS.write_text(text[:pos] + power_parts + text[pos:])


power_attrs_block = '''#ifdef CONFIG_CUSTOM_BATTERY_STATUS
static const uint32_t k_power_attrs[] = {
\tMATTER_ATTR_PS_STATUS, MATTER_ATTR_PS_ORDER, MATTER_ATTR_PS_DESCRIPTION,
\tMATTER_ATTR_PS_BAT_PERCENT_REMAINING, MATTER_ATTR_PS_BAT_CHARGE_LEVEL,
\tMATTER_ATTR_PS_BAT_REPLACEMENT_NEEDED, MATTER_ATTR_PS_BAT_REPLACEABILITY,
\tMATTER_ATTR_PS_BAT_PRESENT, MATTER_ATTR_PS_ENDPOINT_LIST,
\tMATTER_ATTR_GENERATED_CMD_LIST, MATTER_ATTR_ACCEPTED_CMD_LIST,
\tMATTER_ATTR_ATTRIBUTE_LIST, MATTER_ATTR_FEATURE_MAP, MATTER_ATTR_CLUSTER_REVISION,
};
#endif

'''
text = CLUSTERS.read_text()
if "static const uint32_t k_power_attrs[]" not in text:
    pos = text.find("static const uint32_t k_desc_attrs[] = {\n")
    if pos < 0:
        raise SystemExit("k_desc_attrs anchor not found")
    CLUSTERS.write_text(text[:pos] + power_attrs_block + text[pos:])


power_servers_dispatch = '''#ifdef CONFIG_CUSTOM_BATTERY_STATUS
\tif (endpoint == MATTER_ENDPOINT_POWER_SOURCE) {
\t\t*out = k_power_servers;
\t\treturn sizeof(k_power_servers) / sizeof(k_power_servers[0]);
\t}
#endif
'''
insert_before_in_function(
    CLUSTERS,
    "static size_t list_servers(void *ctx, uint16_t endpoint, const uint32_t **out)\n{\n",
    "\tif (endpoint == MATTER_ENDPOINT_LOCK) {\n",
    power_servers_dispatch,
)


power_attrs_dispatch = '''#ifdef CONFIG_CUSTOM_BATTERY_STATUS
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
'''
insert_before_in_function(
    CLUSTERS,
    "static size_t list_attrs(void *ctx, uint16_t endpoint, uint32_t cluster, const uint32_t **out)\n{\n",
    "\tif (endpoint == MATTER_ENDPOINT_LOCK) {\n",
    power_attrs_dispatch,
)


# Port integration.  These may not have been reached by the failed migration.
text = COMMISSION.read_text()
if '#include "battery_status.h"' not in text:
    anchor = '#include "status_led.h" /* the lock LED; a tile tap has to move it too */\n'
    if anchor not in text:
        raise SystemExit("matter_commission status_led include anchor not found")
    text = text.replace(anchor, anchor + '#ifdef CONFIG_CUSTOM_BATTERY_STATUS\n#include "battery_status.h"\n#endif\n', 1)
    COMMISSION.write_text(text)

text = COMMISSION.read_text()
init_block = '''#ifdef CONFIG_CUSTOM_BATTERY_STATUS
\ts_info.battery_percent = battery_status_percent();
#endif
'''
if init_block not in text:
    sig = "int matter_commission_init(void)\n{\n"
    pos = text.find(sig)
    if pos < 0:
        raise SystemExit("matter_commission_init signature not found")
    pos += len(sig)
    COMMISSION.write_text(text[:pos] + init_block + text[pos:])

print("Resumed battery Matter migration and repaired Power Source metadata.")
