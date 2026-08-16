#!/usr/bin/env python3
"""Enable the battery overlay in the normal DWM3001CDK `make build` path."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
CDK_MK = ROOT / "mk" / "cdk.mk"

text = CDK_MK.read_text()
old = "CDK_CONF := overlay-thread.conf$(if $(RELEASE),;overlay-release.conf)$(if $(SMP),;overlay-smp.conf)$(if $(CDK_LTO),;overlay-lto.conf)$(if $(OTLOG),;overlay-otlog.conf)$(if $(ANCHOR),;overlay-anchor.conf)$(if $(SIDE),;overlay-side.conf)\n"
new = "CDK_CONF := overlay-thread.conf;overlay-battery-status.conf$(if $(RELEASE),;overlay-release.conf)$(if $(SMP),;overlay-smp.conf)$(if $(CDK_LTO),;overlay-lto.conf)$(if $(OTLOG),;overlay-otlog.conf)$(if $(ANCHOR),;overlay-anchor.conf)$(if $(SIDE),;overlay-side.conf)\n"

if new in text:
    print("Battery overlay already enabled in make build.")
elif old in text:
    CDK_MK.write_text(text.replace(old, new, 1))
    print("Enabled overlay-battery-status.conf in make build.")
else:
    raise SystemExit("CDK_CONF anchor not found in mk/cdk.mk")
