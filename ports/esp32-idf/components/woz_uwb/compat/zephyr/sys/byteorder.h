/* ESP-IDF compat for <zephyr/sys/byteorder.h> — endian-neutral load/store
 * helpers the pure modules use (ccc_sts.c, aliro codec). Copied from
 * tests/host/shim/zephyr/sys/byteorder.h; results match target regardless of
 * host endianness. */
#ifndef WOZ_ESP_COMPAT_BYTEORDER_H
#define WOZ_ESP_COMPAT_BYTEORDER_H

#include <stdint.h>

static inline uint16_t sys_get_le16(const uint8_t *p)
{
	return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static inline uint32_t sys_get_le32(const uint8_t *p)
{
	return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
	       ((uint32_t)p[3] << 24);
}

static inline uint16_t sys_get_be16(const uint8_t *p)
{
	return (uint16_t)(((uint16_t)p[0] << 8) | (uint16_t)p[1]);
}

static inline uint32_t sys_get_be32(const uint8_t *p)
{
	return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
	       ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

static inline void sys_put_le16(uint16_t v, uint8_t *p)
{
	p[0] = (uint8_t)v;
	p[1] = (uint8_t)(v >> 8);
}

static inline void sys_put_le32(uint32_t v, uint8_t *p)
{
	p[0] = (uint8_t)v;
	p[1] = (uint8_t)(v >> 8);
	p[2] = (uint8_t)(v >> 16);
	p[3] = (uint8_t)(v >> 24);
}

static inline void sys_put_be16(uint16_t v, uint8_t *p)
{
	p[0] = (uint8_t)(v >> 8);
	p[1] = (uint8_t)v;
}

static inline void sys_put_be32(uint32_t v, uint8_t *p)
{
	p[0] = (uint8_t)(v >> 24);
	p[1] = (uint8_t)(v >> 16);
	p[2] = (uint8_t)(v >> 8);
	p[3] = (uint8_t)v;
}

#endif /* WOZ_ESP_COMPAT_BYTEORDER_H */
