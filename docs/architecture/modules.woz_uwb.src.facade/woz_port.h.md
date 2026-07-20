<!-- generated documentation — edit the source, not this file -->
# `modules/woz_uwb/src/facade/woz_port.h`

*No module docstring. First commit: "port: replace the Zephyr compat shims with a neutral woz_port.h contract".*

**used by** [`modules/woz_aliro/src/aliro_reader.c`](../modules.woz_aliro.src/aliro_reader.c.md), [`modules/woz_uwb/src/ccc/ccc_shim_rx.c`](../modules.woz_uwb.src.ccc/ccc_shim_rx.c.md), [`modules/woz_uwb/src/driver/uwb_isr.c`](../modules.woz_uwb.src.driver/uwb_isr.c.md), [`modules/woz_uwb/src/driver/uwb_min.c`](../modules.woz_uwb.src.driver/uwb_min.c.md), [`modules/woz_uwb/src/facade/woz_alloc.h`](woz_alloc.h.md), [`modules/woz_uwb/src/fira/fira_session.c`](../modules.woz_uwb.src.fira/fira_session.c.md)  ·  **discussed in** [`docs/porting.md`](../../porting.md), [`ports/README.md`](../../../ports/README.md), [`ports/esp32-idf/README.md`](../../../ports/esp32-idf/README.md), [`ports/esp32-idf/components/woz_uwb/README.md`](../../../ports/esp32-idf/components/woz_uwb/README.md)

## API

### `static inline void woz_mutex_init(woz_mutex_t *m)`
`modules/woz_uwb/src/facade/woz_port.h:193`

host tests are single-threaded

<details><summary>Undocumented (13)</summary>

- `woz_mutex_t.k_mutex`
- `woz_malloc`
- `woz_calloc`
- `woz_free`
- `woz_uptime_us`
- `timespec`
- `woz_uptime_ms`
- `woz_sleep_ms`
- `woz_sleep_us`
- `woz_cycle_get_32`
- `woz_mutex_t`
- `woz_mutex_lock`
- `woz_mutex_unlock`

</details>
