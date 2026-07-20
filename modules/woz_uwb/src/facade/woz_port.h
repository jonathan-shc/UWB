/*
 * Copyright (c) 2026 asxeem
 * SPDX-License-Identifier: ISC
 * woz_port.h - the platform contract for the UWB engine.
 *
 * This header IS the port specification. Everything the ranging engine needs
 * from an operating system is the eight functions below; a new target is a new
 * branch here plus a DW3000 SPI/GPIO backend. Nothing else.
 *
 * Deliberately absent: work queues, timers, and init hooks. Those are used only
 * by uwb_rxdiag.c, uwb_selftest.c, woz_logfmt.c, woz_logquiet.c and
 * aliro_shell.c, which are Zephyr-only by design and are not in any port's
 * source list. Admitting them here would multiply the port surface for code
 * that never runs on the ranging path.
 *
 * Also absent: the k_work / k_sem / k_poll surface used by dw3000_spi.c and
 * dw3000_hw.c. Every port supplies its own SPI/GPIO backend for those two
 * files, so they never constrain the contract.
 *
 *   woz_malloc/woz_calloc/woz_free  heap
 *   woz_uptime_us                   monotonic microseconds since boot
 *   woz_uptime_ms                   monotonic milliseconds since boot
 *   woz_sleep_ms                    relinquish the CPU for at least ms
 *   woz_sleep_us                    short busy-wait, microseconds (deca_sleep)
 *   woz_cycle_get_32                free-running counter, RX-arm latency probe
 */
#ifndef WOZ_PORT_H
#define WOZ_PORT_H

#include <stddef.h>
#include <stdint.h>

#if defined(__ZEPHYR__)

#include <zephyr/kernel.h>

static inline void *woz_malloc(size_t size)
{
	return k_malloc(size);
}
static inline void *woz_calloc(size_t n, size_t size)
{
	return k_calloc(n, size);
}
static inline void woz_free(void *ptr)
{
	k_free(ptr);
}
static inline int64_t woz_uptime_us(void)
{
	return (int64_t)k_ticks_to_us_floor64(k_uptime_ticks());
}
static inline int64_t woz_uptime_ms(void)
{
	return k_uptime_get();
}
static inline void woz_sleep_ms(int32_t ms)
{
	k_msleep(ms);
}
static inline void woz_sleep_us(int64_t us)
{
	k_usleep((int32_t)us);
}
static inline uint32_t woz_cycle_get_32(void)
{
	return k_cycle_get_32();
}

#elif defined(ESP_PLATFORM)

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdlib.h>

#include "esp_cpu.h"
#include "esp_rom_sys.h"
#include "esp_timer.h"

static inline void *woz_malloc(size_t size)
{
	return malloc(size);
}
static inline void *woz_calloc(size_t n, size_t size)
{
	return calloc(n, size);
}
static inline void woz_free(void *ptr)
{
	free(ptr);
}
static inline int64_t woz_uptime_us(void)
{
	return esp_timer_get_time();
}
static inline int64_t woz_uptime_ms(void)
{
	return esp_timer_get_time() / 1000;
}
static inline void woz_sleep_ms(int32_t ms)
{
	if (ms > 0) {
		vTaskDelay(pdMS_TO_TICKS(ms));
	}
}
static inline void woz_sleep_us(int64_t us)
{
	esp_rom_delay_us((uint32_t)us);
}
static inline uint32_t woz_cycle_get_32(void)
{
	return esp_cpu_get_cycle_count();
}

#elif defined(WOZ_PORT_HOST)

#include <stdlib.h>
#include <time.h>

static inline void *woz_malloc(size_t size)
{
	return malloc(size);
}
static inline void *woz_calloc(size_t n, size_t size)
{
	return calloc(n, size);
}
static inline void woz_free(void *ptr)
{
	free(ptr);
}
static inline int64_t woz_uptime_us(void)
{
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (int64_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}
static inline int64_t woz_uptime_ms(void)
{
	return woz_uptime_us() / 1000;
}
static inline void woz_sleep_ms(int32_t ms)
{
	(void)ms; /* host tests are deterministic; nothing to wait for */
}
static inline void woz_sleep_us(int64_t us)
{
	(void)us;
}
static inline uint32_t woz_cycle_get_32(void)
{
	return (uint32_t)woz_uptime_us(); /* us resolution is plenty for the probe */
}

#else
#error "woz_port.h: no platform backend. Define WOZ_PORT_HOST, or build under Zephyr/ESP-IDF."
#endif

#endif /* WOZ_PORT_H */
