# web-twin — the walk-up digital twin as an interactive page

`index.html` is self-contained (inline JS/CSS, no external requests): open it
straight from the filesystem or serve it from any static host. Drag the phone
toward the door, or press Walk up, and the reader's unlock pipeline reacts:
BLE connect radius, Aliro session bring-up, UWB DS-TWR ranging blocks, the
range-integrity trust gate, the approach controller, the bolt.

The decision logic is a line-cited port of the firmware. The range store and
trust gate mirror `modules/woz_uwb/src/fira/fira_session.c`, the unlock seam
mirrors `modules/woz_uwb/src/facade/woz_uwb_facade.c`, and the median/dwell
approach controller mirrors `ports/esp32/apps/matter-lock/main/app_main.cpp`.
Every decision constant in the page's `FW` table carries a `file:line`
citation into the C tree, and

    python3 web-twin/check_constants.py

fails if any cited value drifts, so the firmware stays the single source of
truth for every number the page uses. Environment knobs that are not firmware
(BLE radio range, noise probability, auth-phase pacing) are marked SIM in the
page. On every load the page replays the scenario `tests/host/test_twin.c`
asserts (legit 234 cm round, Ghost-Peak spoof true-reject, trust earned at
K=3) and shows the result in the footer.
