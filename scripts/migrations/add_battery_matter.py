#!/usr/bin/env python3
"""Add Matter Power Source presentation for the standalone battery feature.

Run after add_battery_status.py. This intentionally mirrors the topology that
was hardware-validated in the earlier fork: a dedicated Power Source utility
endpoint referenced from the root PartsList. The battery measurement remains
board-side and independent from the Matter data model.
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


# Matter Power Source constants. BatPercentRemaining uses half-percent units.
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
    '#define MATTER_ATTR_PS_BAT_REPLACEMENT_NEEDED  0x000Fu\n'
    '#define MATTER_ATTR_PS_BAT_REPLACEABILITY       0x0010u\n'
    '#define MATTER_ATTR_PS_BAT_PRESENT              0x0011u\n'
    '#define MATTER_ATTR_PS_ENDPOINT_LIST            0x001Fu\n'
    '#define MATTER_PS_STATUS_ACTIVE                 1u\n'
    '#define MATTER_PS_FEATURE_BATTERY               0x02u\n'
    '#define MATTER_PS_FEATURE_RECHARGEABLE          0x04u\n'
    '#define MATTER_PS_REPLACEABILITY_USER           2u\n'
    '#endif\n')

replace_once(HDR,
    '#define MATTER_DEVICE_TYPE_LOCK_REV  3u\n',
    '#define MATTER_DEVICE_TYPE_LOCK_REV  3u\n'
    '#ifdef CONFIG_CUSTOM_BATTERY_STATUS\n'
    '#define MATTER_DEVICE_TYPE_POWER_SOURCE     0x0011u\n'
    '#define MATTER_DEVICE_TYPE_POWER_SOURCE_REV 1u\n'
    '#endif\n')

replace_once(HDR,
    '#define MATTER_ENDPOINT_LOCK 1u\n',
    '#define MATTER_ENDPOINT_LOCK 1u\n'
    '#ifdef CONFIG_CUSTOM_BATTERY_STATUS\n'
    '/** Dedicated Power Source utility endpoint; matches the earlier working Home topology. */\n'
    '#define MATTER_ENDPOINT_POWER_SOURCE 2u\n'
    '#endif\n')

replace_once(HDR,
    '\tuint8_t lock_state;\n',
    '\tuint8_t lock_state;\n'
    '#ifdef CONFIG_CUSTOM_BATTERY_STATUS\n'
    '\tuint8_t battery_percent; /* 0..100; Matter encoder converts to 0..200 */\n'
    '#endif\n')

# Dedicated endpoint server list, exactly like the earlier working fork.
replace_once(CLUSTERS,
    'static const uint32_t k_lock_servers[] = {\n\tMATTER_CLUSTER_DESCRIPTOR,\n\tMATTER_CLUSTER_DOOR_LOCK,\n\tMATTER_CLUSTER_APPROACH_DIRECTION,\n};\n',
    'static const uint32_t k_lock_servers[] = {\n\tMATTER_CLUSTER_DESCRIPTOR,\n\tMATTER_CLUSTER_DOOR_LOCK,\n\tMATTER_CLUSTER_APPROACH_DIRECTION,\n};\n'
    '#ifdef CONFIG_CUSTOM_BATTERY_STATUS\n'
    'static const uint32_t k_power_servers[] = {\n'
    '\tMATTER_CLUSTER_DESCRIPTOR,\n'
    '\tMATTER_CLUSTER_POWER_SOURCE,\n'
    '};\n'
    '#endif\n')

# Cluster lookup: Power Source exists only on endpoint 2, never on the lock endpoint.
replace_once(CLUSTERS,
    '\tif (endpoint == MATTER_ENDPOINT_LOCK) {\n\t\treturn cluster == MATTER_CLUSTER_DESCRIPTOR ||\n\t\t       cluster == MATTER_CLUSTER_DOOR_LOCK ||\n\t\t       cluster == MATTER_CLUSTER_APPROACH_DIRECTION;\n\t}\n',
    '\tif (endpoint == MATTER_ENDPOINT_LOCK) {\n\t\treturn cluster == MATTER_CLUSTER_DESCRIPTOR ||\n\t\t       cluster == MATTER_CLUSTER_DOOR_LOCK ||\n\t\t       cluster == MATTER_CLUSTER_APPROACH_DIRECTION;\n\t}\n'
    '#ifdef CONFIG_CUSTOM_BATTERY_STATUS\n'
    '\tif (endpoint == MATTER_ENDPOINT_POWER_SOURCE) {\n'
    '\t\treturn cluster == MATTER_CLUSTER_DESCRIPTOR || cluster == MATTER_CLUSTER_POWER_SOURCE;\n'
    '\t}\n'
    '#endif\n')

# Wildcard endpoint expansion must expose the utility endpoint.
replace_once(CLUSTERS,
    'static const uint16_t k_endpoints[] = {\n\tMATTER_ENDPOINT_ROOT,\n\tMATTER_ENDPOINT_LOCK,\n};\n',
    'static const uint16_t k_endpoints[] = {\n\tMATTER_ENDPOINT_ROOT,\n\tMATTER_ENDPOINT_LOCK,\n'
    '#ifdef CONFIG_CUSTOM_BATTERY_STATUS\n'
    '\tMATTER_ENDPOINT_POWER_SOURCE,\n'
    '#endif\n'
    '};\n')

# Attribute support for endpoint 2. FeatureMap is important: it declares this
# Power Source as a rechargeable battery rather than a generic supply.
status_anchor = '\tif (endpoint == MATTER_ENDPOINT_LOCK) {\n'
status_block = '''#ifdef CONFIG_CUSTOM_BATTERY_STATUS
	if (endpoint == MATTER_ENDPOINT_POWER_SOURCE) {
		if (cluster == MATTER_CLUSTER_DESCRIPTOR) {
			switch (attribute) {
			case MATTER_ATTR_DESC_DEVICE_TYPE_LIST:
			case MATTER_ATTR_DESC_SERVER_LIST:
			case MATTER_ATTR_DESC_CLIENT_LIST:
			case MATTER_ATTR_DESC_PARTS_LIST:
				return MATTER_IM_STATUS_SUCCESS;
			default:
				return MATTER_IM_STATUS_UNSUPPORTED_ATTRIBUTE;
			}
		}
		if (cluster != MATTER_CLUSTER_POWER_SOURCE) {
			return MATTER_IM_STATUS_UNSUPPORTED_CLUSTER;
		}
		switch (attribute) {
		case MATTER_ATTR_PS_STATUS:
		case MATTER_ATTR_PS_ORDER:
		case MATTER_ATTR_PS_DESCRIPTION:
		case MATTER_ATTR_PS_BAT_PERCENT_REMAINING:
		case MATTER_ATTR_PS_BAT_CHARGE_LEVEL:
		case MATTER_ATTR_PS_BAT_REPLACEMENT_NEEDED:
		case MATTER_ATTR_PS_BAT_REPLACEABILITY:
		case MATTER_ATTR_PS_BAT_PRESENT:
		case MATTER_ATTR_PS_ENDPOINT_LIST:
		case MATTER_ATTR_FEATURE_MAP:
			return MATTER_IM_STATUS_SUCCESS;
		default:
			return MATTER_IM_STATUS_UNSUPPORTED_ATTRIBUTE;
		}
	}
#endif
''' + status_anchor
replace_once(CLUSTERS, status_anchor, status_block)

# Attribute list for wildcard reads/subscriptions on the Power Source cluster.
attrs_anchor = 'static const uint32_t k_desc_attrs[] = {\n'
power_attrs = '''#ifdef CONFIG_CUSTOM_BATTERY_STATUS
static const uint32_t k_power_attrs[] = {
	MATTER_ATTR_PS_STATUS,
	MATTER_ATTR_PS_ORDER,
	MATTER_ATTR_PS_DESCRIPTION,
	MATTER_ATTR_PS_BAT_PERCENT_REMAINING,
	MATTER_ATTR_PS_BAT_CHARGE_LEVEL,
	MATTER_ATTR_PS_BAT_REPLACEMENT_NEEDED,
	MATTER_ATTR_PS_BAT_REPLACEABILITY,
	MATTER_ATTR_PS_BAT_PRESENT,
	MATTER_ATTR_PS_ENDPOINT_LIST,
	MATTER_ATTR_FEATURE_MAP,
};
#endif

''' + attrs_anchor
replace_once(CLUSTERS, attrs_anchor, power_attrs)

# Dedicated endpoint value encoder. This is the key topology difference from
# the failed attempt that attached Power Source directly to endpoint 1.
lock_value_anchor = '''static void lock_attr_value(const struct matter_device_info *info, uint32_t cluster,
			    uint32_t attribute, struct matter_tlv_writer *w, matter_tlv_tag_t tag)
'''
power_value = '''#ifdef CONFIG_CUSTOM_BATTERY_STATUS
/** Endpoint 2: dedicated Power Source utility device. */
static void power_attr_value(const struct matter_device_info *info, uint32_t cluster,
			     uint32_t attribute, struct matter_tlv_writer *w, matter_tlv_tag_t tag)
{
	size_t i;

	if (cluster == MATTER_CLUSTER_DESCRIPTOR) {
		switch (attribute) {
		case MATTER_ATTR_DESC_DEVICE_TYPE_LIST:
			(void)matter_tlv_start_container(w, tag, MATTER_TLV_ARRAY);
			(void)matter_tlv_start_container(w, MATTER_TLV_ANON, MATTER_TLV_STRUCTURE);
			(void)matter_tlv_put_u64(w, MATTER_TLV_CTX(TAG_DEVTYPE_TYPE),
						 MATTER_DEVICE_TYPE_POWER_SOURCE);
			(void)matter_tlv_put_u64(w, MATTER_TLV_CTX(TAG_DEVTYPE_REVISION),
						 MATTER_DEVICE_TYPE_POWER_SOURCE_REV);
			(void)matter_tlv_end_container(w);
			(void)matter_tlv_end_container(w);
			return;
		case MATTER_ATTR_DESC_SERVER_LIST:
			(void)matter_tlv_start_container(w, tag, MATTER_TLV_ARRAY);
			for (i = 0u; i < sizeof(k_power_servers) / sizeof(k_power_servers[0]); i++) {
				(void)matter_tlv_put_u64(w, MATTER_TLV_ANON, k_power_servers[i]);
			}
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

	if (cluster != MATTER_CLUSTER_POWER_SOURCE) {
		return;
	}
	switch (attribute) {
	case MATTER_ATTR_PS_STATUS:
		(void)matter_tlv_put_u64(w, tag, MATTER_PS_STATUS_ACTIVE);
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
		(void)matter_tlv_put_u64(w, tag,
					 info->battery_percent <= 10u ? 2u :
					 info->battery_percent <= 20u ? 1u : 0u);
		return;
	case MATTER_ATTR_PS_BAT_REPLACEMENT_NEEDED:
		(void)matter_tlv_put_bool(w, tag, false);
		return;
	case MATTER_ATTR_PS_BAT_REPLACEABILITY:
		(void)matter_tlv_put_u64(w, tag, MATTER_PS_REPLACEABILITY_USER);
		return;
	case MATTER_ATTR_PS_BAT_PRESENT:
		(void)matter_tlv_put_bool(w, tag, true);
		return;
	case MATTER_ATTR_PS_ENDPOINT_LIST:
		(void)matter_tlv_start_container(w, tag, MATTER_TLV_ARRAY);
		(void)matter_tlv_put_u64(w, MATTER_TLV_ANON, MATTER_ENDPOINT_LOCK);
		(void)matter_tlv_end_container(w);
		return;
	case MATTER_ATTR_FEATURE_MAP:
		(void)matter_tlv_put_u64(w, tag,
					 MATTER_PS_FEATURE_BATTERY | MATTER_PS_FEATURE_RECHARGEABLE);
		return;
	default:
		return;
	}
}
#endif

''' + lock_value_anchor
replace_once(CLUSTERS, lock_value_anchor, power_value)

# Dispatch endpoint 2 before endpoint 1/root handling.
value_dispatch_anchor = '''	if (endpoint == MATTER_ENDPOINT_LOCK) {
		lock_attr_value(info, cluster, attribute, w, tag);
		return;
	}
'''
value_dispatch = '''#ifdef CONFIG_CUSTOM_BATTERY_STATUS
	if (endpoint == MATTER_ENDPOINT_POWER_SOURCE) {
		power_attr_value(info, cluster, attribute, w, tag);
		return;
	}
#endif
''' + value_dispatch_anchor
replace_once(CLUSTERS, value_dispatch_anchor, value_dispatch)

# Root PartsList associates the Power Source endpoint with this node/accessory.
parts_anchor = '\t\t\t(void)matter_tlv_put_u64(w, MATTER_TLV_ANON, MATTER_ENDPOINT_LOCK);\n\t\t\t(void)matter_tlv_end_container(w);\n'
parts_block = '\t\t\t(void)matter_tlv_put_u64(w, MATTER_TLV_ANON, MATTER_ENDPOINT_LOCK);\n#ifdef CONFIG_CUSTOM_BATTERY_STATUS\n\t\t\t(void)matter_tlv_put_u64(w, MATTER_TLV_ANON, MATTER_ENDPOINT_POWER_SOURCE);\n#endif\n\t\t\t(void)matter_tlv_end_container(w);\n'
replace_once(CLUSTERS, parts_anchor, parts_block)

# Wildcard server and attribute expansion for endpoint 2.
servers_anchor = '''	if (endpoint == MATTER_ENDPOINT_LOCK) {
		*out = k_lock_servers;
		return sizeof(k_lock_servers) / sizeof(k_lock_servers[0]);
	}
'''
servers_block = servers_anchor + '''#ifdef CONFIG_CUSTOM_BATTERY_STATUS
	if (endpoint == MATTER_ENDPOINT_POWER_SOURCE) {
		*out = k_power_servers;
		return sizeof(k_power_servers) / sizeof(k_power_servers[0]);
	}
#endif
'''
replace_once(CLUSTERS, servers_anchor, servers_block)

attrs_dispatch_anchor = '''	if (endpoint == MATTER_ENDPOINT_LOCK) {
		if (cluster == MATTER_CLUSTER_DESCRIPTOR) {
'''
attrs_dispatch_block = '''#ifdef CONFIG_CUSTOM_BATTERY_STATUS
	if (endpoint == MATTER_ENDPOINT_POWER_SOURCE) {
		if (cluster == MATTER_CLUSTER_DESCRIPTOR) {
			*out = k_desc_attrs;
			return sizeof(k_desc_attrs) / sizeof(k_desc_attrs[0]);
		}
		if (cluster == MATTER_CLUSTER_POWER_SOURCE) {
			*out = k_power_attrs;
			return sizeof(k_power_attrs) / sizeof(k_power_attrs[0]);
		}
		return 0u;
	}
#endif
''' + attrs_dispatch_anchor
replace_once(CLUSTERS, attrs_dispatch_anchor, attrs_dispatch_block)

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

print("Applied dedicated Matter Power Source endpoint integration.")
