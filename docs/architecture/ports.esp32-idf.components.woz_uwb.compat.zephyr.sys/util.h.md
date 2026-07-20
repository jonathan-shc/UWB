<!-- generated documentation — edit the source, not this file -->
# `ports/esp32-idf/components/woz_uwb/compat/zephyr/sys/util.h`

ESP-IDF compat for <zephyr/sys/util.h> — container/compare macros plus
IS_ENABLED (the on-silicon build compiles dw3000_device.c, which uses it;
the host shim did not need it because it stubs the driver).

**used by** [`modules/woz_uwb/src/aliro/aliro_uwb_msg.c`](../modules.woz_uwb.src.aliro/aliro_uwb_msg.c.md), [`modules/woz_uwb/src/ccc/cherry_ccc_shim.c`](../modules.woz_uwb.src.ccc/cherry_ccc_shim.c.md)
