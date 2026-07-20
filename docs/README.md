<!-- generated documentation — edit the source, not this file -->
# openaliro

**100 subsystems in 21 directories · 725/772 symbols documented (93%)**

**Start here:** [`modules/woz_uwb/src/aliro/aliro_uwb_msg.c`](architecture/modules.woz_uwb.src.aliro/aliro_uwb_msg.c.md), [`modules/woz_uwb/src/aliro/aliro_uwb_session.c`](architecture/modules.woz_uwb.src.aliro/aliro_uwb_session.c.md), [`modules/woz_aliro/src/aliro_ranging.c`](architecture/modules.woz_aliro.src/aliro_ranging.c.md) — the doors into the codebase (nothing else imports them).

```mermaid
flowchart LR
  modules.woz_aliro.src --> modules.woz_aliro.include
  modules.woz_aliro.src --> modules.woz_uwb.src.aliro.include.aliro_uwb_adapter
  modules.woz_aliro.src --> modules.woz_uwb.src.aliro.include.cherry
  modules.woz_aliro.src --> modules.woz_uwb.src.facade
  modules.woz_uwb.src.aliro --> modules.woz_uwb.src.aliro.include.aliro_uwb_adapter
  modules.woz_uwb.src.aliro --> modules.woz_uwb.src.aliro.include.cherry
  modules.woz_uwb.src.aliro --> modules.woz_uwb.src.ccc
  modules.woz_uwb.src.aliro --> modules.woz_uwb.src.facade
  modules.woz_uwb.src.aliro.include.aliro_uwb_adapter --> modules.woz_uwb.src.aliro.include.cherry
  modules.woz_uwb.src.ccc --> modules.woz_uwb.src.aliro.include.cherry
  modules.woz_uwb.src.ccc --> modules.woz_uwb.src.driver
  modules.woz_uwb.src.ccc --> modules.woz_uwb.src.facade
  modules.woz_uwb.src.ccc --> modules.woz_uwb.src.fira
  modules.woz_uwb.src.driver --> modules.woz_uwb.src.ccc
  modules.woz_uwb.src.driver --> modules.woz_uwb.src.facade
  modules.woz_uwb.src.driver --> modules.woz_uwb.src.fira
  modules.woz_uwb.src.facade --> modules.woz_uwb.src.ccc
  modules.woz_uwb.src.facade --> modules.woz_uwb.src.fira
  modules.woz_uwb.src.fira --> modules.woz_uwb.src.ccc
  modules.woz_uwb.src.fira --> modules.woz_uwb.src.facade
  modules.woz_uwb.src.shell --> modules.woz_uwb.src.ccc
  modules.woz_uwb.src.shell --> modules.woz_uwb.src.driver
  modules.woz_uwb.src.shell --> modules.woz_uwb.src.fira
  ports.esp32-matter.main --> ports.esp32-matter.main.lock
```

## Directories

| directory | subsystems | documented |
|---|---|---|
| [`./`](architecture/root/README.md) | 4 | 14/14 (100%) |
| [`integration/homeassistant/`](architecture/integration.homeassistant/README.md) | 1 | 4/5 (80%) |
| [`modules/woz_aliro/include/`](architecture/modules.woz_aliro.include/README.md) | 5 | 9/9 (100%) |
| [`modules/woz_aliro/src/`](architecture/modules.woz_aliro.src/README.md) | 10 | 115/123 (93%) |
| [`modules/woz_aliro_ecp/src/`](architecture/modules.woz_aliro_ecp.src/README.md) | 1 | 5/5 (100%) |
| [`modules/woz_uwb/src/aliro/`](architecture/modules.woz_uwb.src.aliro/README.md) | 10 | 106/118 (89%) |
| [`modules/woz_uwb/src/aliro/include/aliro_uwb_adapter/`](architecture/modules.woz_uwb.src.aliro.include.aliro_uwb_adapter/README.md) | 2 | 14/17 (82%) |
| [`modules/woz_uwb/src/aliro/include/cherry/`](architecture/modules.woz_uwb.src.aliro.include.cherry/README.md) | 4 | 33/36 (91%) |
| [`modules/woz_uwb/src/ccc/`](architecture/modules.woz_uwb.src.ccc/README.md) | 17 | 141/142 (99%) |
| [`modules/woz_uwb/src/driver/`](architecture/modules.woz_uwb.src.driver/README.md) | 7 | 43/43 (100%) |
| [`modules/woz_uwb/src/facade/`](architecture/modules.woz_uwb.src.facade/README.md) | 11 | 34/47 (72%) |
| [`modules/woz_uwb/src/fira/`](architecture/modules.woz_uwb.src.fira/README.md) | 3 | 10/10 (100%) |
| [`modules/woz_uwb/src/shell/`](architecture/modules.woz_uwb.src.shell/README.md) | 1 | 12/12 (100%) |
| [`ports/esp32-idf/components/aliro_ble/`](architecture/ports.esp32-idf.components.aliro_ble/README.md) | 1 | 41/42 (97%) |
| [`ports/esp32-idf/components/aliro_reader/`](architecture/ports.esp32-idf.components.aliro_reader/README.md) | 1 | 5/5 (100%) |
| [`ports/esp32-idf/components/woz_uwb/port/`](architecture/ports.esp32-idf.components.woz_uwb.port/README.md) | 4 | 30/30 (100%) |
| [`ports/esp32-idf/main/`](architecture/ports.esp32-idf.main/README.md) | 3 | 16/16 (100%) |
| [`ports/esp32-matter/main/`](architecture/ports.esp32-matter.main/README.md) | 7 | 27/27 (100%) |
| [`ports/esp32-matter/main/lock/`](architecture/ports.esp32-matter.main.lock/README.md) | 5 | 60/60 (100%) |
| [`ports/esp32s3/sample/src/`](architecture/ports.esp32s3.sample.src/README.md) | 1 | 2/2 (100%) |
| [`tools/`](architecture/tools/README.md) | 2 | 4/9 (44%) |

## Hotspots

*Mined from git history as of `2d4b6b1`.*

**Most-changed:** [`modules/woz_uwb/src/ccc/ccc_shim_rx.c`](architecture/modules.woz_uwb.src.ccc/ccc_shim_rx.c.md) (12 commits), [`ports/esp32-matter/main/app_main.cpp`](architecture/ports.esp32-matter.main/app_main.cpp.md) (9 commits), [`build.sh`](architecture/root/build.sh.md) (8 commits), [`bootstrap.sh`](architecture/root/bootstrap.sh.md) (7 commits), [`ports/esp32-idf/components/aliro_ble/aliro_ble.c`](architecture/ports.esp32-idf.components.aliro_ble/aliro_ble.c.md) (7 commits).

**Change together without importing each other:**

- [`ports/esp32-matter/main/app_main.cpp`](architecture/ports.esp32-matter.main/app_main.cpp.md) ↔ [`ports/esp32-matter/main/lock/door_lock_manager.cpp`](architecture/ports.esp32-matter.main.lock/door_lock_manager.cpp.md) (3 shared commits)
