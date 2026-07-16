/* ESP-IDF compat for <zephyr/kernel.h> — the k_* surface the woz_uwb engine +
 * dw3000 platform glue touch, backed by FreeRTOS + esp_timer. On-silicon twin
 * of tests/host/shim/zephyr/kernel.h (which is libc-backed for host tests).
 * Additive: the shared engine .c files compile unmodified because this resolves
 * first on the include path. See ports/esp32-idf/README.md. */
#ifndef WOZ_ESP_COMPAT_KERNEL_H
#define WOZ_ESP_COMPAT_KERNEL_H

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_rom_sys.h" /* esp_rom_delay_us */
#include "esp_cpu.h"     /* esp_cpu_get_cycle_count */

/* --- heap (woz_alloc.h) --- */
static inline void *k_malloc(size_t size) { return malloc(size); }
static inline void *k_calloc(size_t n, size_t size) { return calloc(n, size); }
static inline void k_free(void *ptr) { free(ptr); }

/* --- monotonic clock ---
 * woz_alloc.h models a tick as one microsecond, so keep ticks == us. */
static inline int64_t k_uptime_ticks(void) { return esp_timer_get_time(); }
static inline int64_t k_ticks_to_us_floor64(int64_t ticks) { return ticks; }
static inline int64_t k_uptime_get(void) { return esp_timer_get_time() / 1000; }
static inline uint32_t k_uptime_get_32(void) { return (uint32_t)k_uptime_get(); }

/* CPU cycle counter (ccc_shim_rx.c per-arm timestamp). */
static inline uint32_t k_cycle_get_32(void) { return esp_cpu_get_cycle_count(); }

/* --- sleeps / busy-waits --- */
static inline void k_busy_wait(uint32_t usec) { esp_rom_delay_us(usec); }
static inline void k_usleep(int64_t usec) { esp_rom_delay_us((uint32_t)usec); }
static inline void k_msleep(int32_t ms)
{
	if (ms <= 0) {
		return;
	}
	vTaskDelay(pdMS_TO_TICKS(ms));
}

#endif /* WOZ_ESP_COMPAT_KERNEL_H */
