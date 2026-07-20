/* ESP-IDF compat for <zephyr/sys/byteorder.h> — endian-neutral load/store
 * helpers the pure modules use (ccc_sts.c, aliro codec). Copied from
 * tests/host/shim/zephyr/sys/byteorder.h; results match target regardless of
 * host endianness. */
#ifndef WOZ_ESP_COMPAT_BYTEORDER_H
#define WOZ_ESP_COMPAT_BYTEORDER_H

#include <stdint.h>

// Read a 16-bit little-endian value from a byte buffer.
static inline uint16_t sys_get_le16(const uint8_t *p)
{
	return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

// Read a 32-bit little-endian value from a byte buffer.
static inline uint32_t sys_get_le32(const uint8_t *p)
{
	return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
	       ((uint32_t)p[3] << 24);
}

// Read a 16-bit big-endian value from a byte buffer.
static inline uint16_t sys_get_be16(const uint8_t *p)
{
	return (uint16_t)(((uint16_t)p[0] << 8) | (uint16_t)p[1]);
}

// Read a 32-bit big-endian value from a byte buffer.
static inline uint32_t sys_get_be32(const uint8_t *p)
{
	return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
	       ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

// Write a 16-bit value to a byte buffer in little-endian order.
static inline void sys_put_le16(uint16_t v, uint8_t *p)
{
	p[0] = (uint8_t)v;
	p[1] = (uint8_t)(v >> 8);
}

// Write a 32-bit value to a byte buffer in little-endian order.
static inline void sys_put_le32(uint32_t v, uint8_t *p)
{
	p[0] = (uint8_t)v;
	p[1] = (uint8_t)(v >> 8);
	p[2] = (uint8_t)(v >> 16);
	p[3] = (uint8_t)(v >> 24);
}

// Write a 16-bit value to a byte buffer in big-endian order.
static inline void sys_put_be16(uint16_t v, uint8_t *p)
{
	p[0] = (uint8_t)(v >> 8);
	p[1] = (uint8_t)v;
}

// Write a 32-bit value to a byte buffer in big-endian order.
static inline void sys_put_be32(uint32_t v, uint8_t *p)
{
	p[0] = (uint8_t)(v >> 24);
	p[1] = (uint8_t)(v >> 16);
	p[2] = (uint8_t)(v >> 8);
	p[3] = (uint8_t)v;
}

#endif /* WOZ_ESP_COMPAT_BYTEORDER_H */
