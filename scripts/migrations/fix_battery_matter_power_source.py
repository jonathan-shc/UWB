#!/usr/bin/env python3
"""Repair the standalone battery Matter Power Source presentation.

Run after add_battery_status.py and add_battery_matter.py.  This is deliberately
safe to run on a working tree where the original battery migration has already
been applied: it upgrades the generated Power Source cluster in place.

The first battery migration exposed BatPercentRemaining, but it omitted the
standard global cluster attributes.  It also advertised the RECHG feature
without implementing that feature's mandatory attributes.  Some controllers
(including Apple Home) use the cluster metadata to decide whether to surface a
battery service, so keep this endpoint deliberately small and conformant:
BAT only, Power Source cluster revision 3, no commands, and complete globals.
"""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
HDR = ROOT / "modules" / "ultrawidelock_matter" / "include" / "matter_clusters.h"
CLUSTERS = ROOT / "modules" / "ultrawidelock_matter" / "src" / "matter_clusters.c"


def replace_once(path: Path, old: str, new: str) -> None:
    text = path.read_text()
    if new in text:
        return
    if old not in text:
        raise SystemExit(f"anchor not found in {path}: {old[:160]!r}")
    path.write_text(text.replace(old, new, 1))


# Advertise only the BAT feature.  RECHG would require BatChargeState and
# BatFunctionalWhileCharging, which this board integration does not provide.
# Cluster revision 3 is the Power Source revision in the Matter data model.
replace_once(
    HDR,
    '''#define MATTER_PS_FEATURE_BATTERY               0x02u
#define MATTER_PS_FEATURE_RECHARGEABLE          0x04u
#define MATTER_PS_REPLACEABILITY_USER           2u
''',
    '''#define MATTER_PS_FEATURE_BATTERY               0x02u
#define MATTER_PS_CLUSTER_REVISION              3u
#define MATTER_PS_REPLACEABILITY_NOT_REPLACEABLE 1u
''',
)

# A wildcard/global read must see the standard cluster metadata as supported.
replace_once(
    CLUSTERS,
    '''\t\tcase MATTER_ATTR_PS_ENDPOINT_LIST:
\t\tcase MATTER_ATTR_FEATURE_MAP:
\t\t\treturn MATTER_IM_STATUS_SUCCESS;
''',
    '''\t\tcase MATTER_ATTR_PS_ENDPOINT_LIST:
\t\tcase MATTER_ATTR_FEATURE_MAP:
\t\tcase MATTER_ATTR_CLUSTER_REVISION:
\t\tcase MATTER_ATTR_ATTRIBUTE_LIST:
\t\tcase MATTER_ATTR_ACCEPTED_CMD_LIST:
\t\tcase MATTER_ATTR_GENERATED_CMD_LIST:
\t\t\treturn MATTER_IM_STATUS_SUCCESS;
''',
)

# BatReplaceability is mandatory for BAT.  Do not claim the REPLC feature (and
# its additional mandatory replacement metadata) when the integration has no
# battery model yet.
replace_once(
    CLUSTERS,
    '''\tcase MATTER_ATTR_PS_BAT_REPLACEABILITY: (void)matter_tlv_put_u64(w, tag, MATTER_PS_REPLACEABILITY_USER); return;
''',
    '''\tcase MATTER_ATTR_PS_BAT_REPLACEABILITY: (void)matter_tlv_put_u64(w, tag, MATTER_PS_REPLACEABILITY_NOT_REPLACEABLE); return;
''',
)

# Encode the global metadata directly here.  k_power_attrs is declared later in
# this compact hand-written server, so AttributeList is emitted explicitly to
# avoid introducing a declaration-order dependency.
replace_once(
    CLUSTERS,
    '''\tcase MATTER_ATTR_FEATURE_MAP: (void)matter_tlv_put_u64(w, tag, MATTER_PS_FEATURE_BATTERY | MATTER_PS_FEATURE_RECHARGEABLE); return;
\tdefault: return;
''',
    '''\tcase MATTER_ATTR_FEATURE_MAP:
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
\tdefault: return;
''',
)

# Keep list_attrs in sync with the attribute values above.  Controllers use
# this list to expand wildcard reads and to discover cluster capabilities.
replace_once(
    CLUSTERS,
    '''\tMATTER_ATTR_PS_BAT_REPLACEMENT_NEEDED, MATTER_ATTR_PS_BAT_REPLACEABILITY,
\tMATTER_ATTR_PS_BAT_PRESENT, MATTER_ATTR_PS_ENDPOINT_LIST, MATTER_ATTR_FEATURE_MAP,
''',
    '''\tMATTER_ATTR_PS_BAT_REPLACEMENT_NEEDED, MATTER_ATTR_PS_BAT_REPLACEABILITY,
\tMATTER_ATTR_PS_BAT_PRESENT, MATTER_ATTR_PS_ENDPOINT_LIST,
\tMATTER_ATTR_GENERATED_CMD_LIST, MATTER_ATTR_ACCEPTED_CMD_LIST,
\tMATTER_ATTR_ATTRIBUTE_LIST, MATTER_ATTR_FEATURE_MAP, MATTER_ATTR_CLUSTER_REVISION,
''',
)

print("Repaired Matter Power Source globals and feature conformance.")
