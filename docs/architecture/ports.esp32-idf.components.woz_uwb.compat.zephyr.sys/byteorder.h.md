<!-- generated documentation — edit the source, not this file -->
# `ports/esp32-idf/components/woz_uwb/compat/zephyr/sys/byteorder.h`

ESP-IDF compat for <zephyr/sys/byteorder.h> — endian-neutral load/store
helpers the pure modules use (ccc_sts.c, aliro codec). Copied from
tests/host/shim/zephyr/sys/byteorder.h; results match target regardless of
host endianness.

**used by** [`modules/woz_uwb/src/ccc/ccc_shim_rx.c`](../modules.woz_uwb.src.ccc/ccc_shim_rx.c.md), [`modules/woz_uwb/src/ccc/ccc_shim_wrap.c`](../modules.woz_uwb.src.ccc/ccc_shim_wrap.c.md), [`modules/woz_uwb/src/ccc/ccc_sts.c`](../modules.woz_uwb.src.ccc/ccc_sts.c.md)

## API

### `static inline uint16_t sys_get_le16(const uint8_t *p)`
`ports/esp32-idf/components/woz_uwb/compat/zephyr/sys/byteorder.h:11`

Read a 16-bit little-endian value from a byte buffer.

### `static inline uint32_t sys_get_le32(const uint8_t *p)`
`ports/esp32-idf/components/woz_uwb/compat/zephyr/sys/byteorder.h:17`

Read a 32-bit little-endian value from a byte buffer.

### `static inline uint16_t sys_get_be16(const uint8_t *p)`
`ports/esp32-idf/components/woz_uwb/compat/zephyr/sys/byteorder.h:24`

Read a 16-bit big-endian value from a byte buffer.

### `static inline uint32_t sys_get_be32(const uint8_t *p)`
`ports/esp32-idf/components/woz_uwb/compat/zephyr/sys/byteorder.h:30`

Read a 32-bit big-endian value from a byte buffer.

### `static inline void sys_put_le16(uint16_t v, uint8_t *p)`
`ports/esp32-idf/components/woz_uwb/compat/zephyr/sys/byteorder.h:37`

Write a 16-bit value to a byte buffer in little-endian order.

### `static inline void sys_put_le32(uint32_t v, uint8_t *p)`
`ports/esp32-idf/components/woz_uwb/compat/zephyr/sys/byteorder.h:44`

Write a 32-bit value to a byte buffer in little-endian order.

### `static inline void sys_put_be16(uint16_t v, uint8_t *p)`
`ports/esp32-idf/components/woz_uwb/compat/zephyr/sys/byteorder.h:53`

Write a 16-bit value to a byte buffer in big-endian order.

### `static inline void sys_put_be32(uint32_t v, uint8_t *p)`
`ports/esp32-idf/components/woz_uwb/compat/zephyr/sys/byteorder.h:60`

Write a 32-bit value to a byte buffer in big-endian order.
