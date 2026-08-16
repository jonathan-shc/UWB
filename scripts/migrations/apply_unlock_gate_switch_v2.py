#!/usr/bin/env python3
"""Apply the standalone unlock-gate integration with a portable Matter build.

Runs the original integration migration, then keeps gate state inside the
platform-independent Matter module. This avoids Zephyr-only IS_ENABLED() in
matter_clusters.c and avoids coupling the Matter module to app/src.
"""
from pathlib import Path
import runpy

ROOT = Path(__file__).resolve().parents[2]
APP = ROOT / "apps" / "dwm3001cdk-lock"
MAIN = APP / "src" / "main.c"
APP_CMAKE = APP / "CMakeLists.txt"
CLU = ROOT / "modules" / "ultrawidelock_matter" / "src" / "matter_clusters.c"

runpy.run_path(str(Path(__file__).with_name("apply_unlock_gate_switch.py")), run_name="__main__")

# Gate state now lives in modules/ultrawidelock_matter/src/unlock_gate.c.
cmake = APP_CMAKE.read_text()
cmake = cmake.replace(
    "target_sources_ifdef(CONFIG_ULTRAWIDELOCK_UNLOCK_GATE_SWITCH app PRIVATE src/unlock_gate.c)\n",
    "",
)
APP_CMAKE.write_text(cmake)

# matter_clusters.c is deliberately platform-independent C11 and does not
# include Zephyr's sys/util.h, so use the standard preprocessor form here.
for path in (CLU, MAIN):
    text = path.read_text()
    text = text.replace(
        "#if IS_ENABLED(CONFIG_ULTRAWIDELOCK_UNLOCK_GATE_SWITCH)",
        "#ifdef CONFIG_ULTRAWIDELOCK_UNLOCK_GATE_SWITCH",
    )
    path.write_text(text)

print("Applied standalone unlock gate v2 (portable Matter module layout).")
