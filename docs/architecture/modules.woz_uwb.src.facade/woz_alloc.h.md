<!-- generated documentation — edit the source, not this file -->
# `modules/woz_uwb/src/facade/woz_alloc.h`

Memory allocation and timing facade: qmalloc, qcalloc, qfree wrap the platform heap;
qrtc_get_us returns monotonic microseconds since boot.

**depends on** [`modules/woz_uwb/src/facade/woz_port.h`](woz_port.h.md)  ·  **used by** [`modules/woz_uwb/src/aliro/aliro_uwb_adapter.c`](../modules.woz_uwb.src.aliro/aliro_uwb_adapter.c.md), [`modules/woz_uwb/src/aliro/aliro_uwb_msg.c`](../modules.woz_uwb.src.aliro/aliro_uwb_msg.c.md), [`modules/woz_uwb/src/aliro/aliro_uwb_msg_builder.c`](../modules.woz_uwb.src.aliro/aliro_uwb_msg_builder.c.md), [`modules/woz_uwb/src/aliro/aliro_uwb_session.c`](../modules.woz_uwb.src.aliro/aliro_uwb_session.c.md), [`modules/woz_uwb/src/ccc/cherry_ccc_shim.c`](../modules.woz_uwb.src.ccc/cherry_ccc_shim.c.md), [`modules/woz_uwb/src/driver/uwb_rxdiag.c`](../modules.woz_uwb.src.driver/uwb_rxdiag.c.md)  ·  **discussed in** [`docs/porting-esp32.md`](../../porting-esp32.md)

## API

### `static inline void *qmalloc(size_t size)`
`modules/woz_uwb/src/facade/woz_alloc.h:17`

Allocate size bytes; wrapper around woz_malloc.

### `static inline void *qcalloc(size_t nb_items, size_t item_size)`
`modules/woz_uwb/src/facade/woz_alloc.h:24`

Allocate and zero-initialize nb_items elements of item_size bytes each; wrapper around
woz_calloc.

### `static inline void qfree(void *ptr)`
`modules/woz_uwb/src/facade/woz_alloc.h:30`

Deallocate memory previously allocated by qmalloc or qcalloc; wrapper around woz_free.

### `static inline int64_t qrtc_get_us(void)`
`modules/woz_uwb/src/facade/woz_alloc.h:36`

Monotonic microseconds since boot.
