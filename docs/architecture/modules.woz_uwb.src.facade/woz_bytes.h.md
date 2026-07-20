<!-- generated documentation — edit the source, not this file -->
# `modules/woz_uwb/src/facade/woz_bytes.h`

**used by** [`modules/woz_uwb/src/ccc/ccc_shim_rx.c`](../modules.woz_uwb.src.ccc/ccc_shim_rx.c.md), [`modules/woz_uwb/src/ccc/ccc_shim_wrap.c`](../modules.woz_uwb.src.ccc/ccc_shim_wrap.c.md), [`modules/woz_uwb/src/ccc/ccc_sts.c`](../modules.woz_uwb.src.ccc/ccc_sts.c.md)  ·  **discussed in** [`docs/porting.md`](../../porting.md), [`ports/esp32-idf/components/woz_uwb/README.md`](../../../ports/esp32-idf/components/woz_uwb/README.md)

## API

### `static inline uint16_t sys_get_le16(const uint8_t *p)`
`modules/woz_uwb/src/facade/woz_bytes.h:28`

Read a 16-bit little-endian value from a byte buffer.

### `static inline uint32_t sys_get_le32(const uint8_t *p)`
`modules/woz_uwb/src/facade/woz_bytes.h:34`

Read a 32-bit little-endian value from a byte buffer.

### `static inline uint16_t sys_get_be16(const uint8_t *p)`
`modules/woz_uwb/src/facade/woz_bytes.h:41`

Read a 16-bit big-endian value from a byte buffer.

### `static inline uint32_t sys_get_be32(const uint8_t *p)`
`modules/woz_uwb/src/facade/woz_bytes.h:47`

Read a 32-bit big-endian value from a byte buffer.

### `static inline void sys_put_le16(uint16_t v, uint8_t *p)`
`modules/woz_uwb/src/facade/woz_bytes.h:54`

Write a 16-bit value to a byte buffer in little-endian order.

### `static inline void sys_put_le32(uint32_t v, uint8_t *p)`
`modules/woz_uwb/src/facade/woz_bytes.h:61`

Write a 32-bit value to a byte buffer in little-endian order.

### `static inline void sys_put_be16(uint16_t v, uint8_t *p)`
`modules/woz_uwb/src/facade/woz_bytes.h:70`

Write a 16-bit value to a byte buffer in big-endian order.

### `static inline void sys_put_be32(uint32_t v, uint8_t *p)`
`modules/woz_uwb/src/facade/woz_bytes.h:77`

Write a 32-bit value to a byte buffer in big-endian order.
