<!-- generated documentation — edit the source, not this file -->
# `modules/woz_uwb/src/facade/trace.h`

@file trace.h — Structured [WOZ_TRACE] emit helpers, gated on CONFIG_WOZ_E2E_TRACE.

**depends on** [`ports/esp32-idf/components/woz_uwb/compat/zephyr/logging/log.h`](../ports.esp32-idf.components.woz_uwb.compat.zephyr.logging/log.h.md), [`ports/esp32-idf/components/woz_uwb/compat/zephyr/sys/printk.h`](../ports.esp32-idf.components.woz_uwb.compat.zephyr.sys/printk.h.md)  ·  **used by** [`modules/woz_uwb/src/driver/uwb_isr.c`](../modules.woz_uwb.src.driver/uwb_isr.c.md)

## API

### `static inline const char *woz_trace_hex8(char buf[WOZ_TRACE_HEX8_LEN], const uint8_t *bytes, size_t len)`
`modules/woz_uwb/src/facade/trace.h:49`

@brief Stub that touches @p buf and @p bytes so neither becomes unused.
