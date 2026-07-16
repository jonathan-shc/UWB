/* ESP-IDF compat for <zephyr/sys/util.h> — container/compare macros plus
 * IS_ENABLED (the on-silicon build compiles dw3000_device.c, which uses it;
 * the host shim did not need it because it stubs the driver). */
#ifndef WOZ_ESP_COMPAT_UTIL_H
#define WOZ_ESP_COMPAT_UTIL_H

#include <stddef.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif
#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef CLAMP
#define CLAMP(x, lo, hi) MIN(MAX((x), (lo)), (hi))
#endif
#ifndef BIT
#define BIT(n) (1UL << (n))
#endif
#ifndef DIV_ROUND_UP
#define DIV_ROUND_UP(n, d) (((n) + (d) - 1) / (d))
#endif
#ifndef ROUND_UP
#define ROUND_UP(x, align) (DIV_ROUND_UP((x), (align)) * (align))
#endif
#ifndef CONTAINER_OF
#define CONTAINER_OF(ptr, type, field) \
	((type *)(((char *)(ptr)) - offsetof(type, field)))
#endif
#ifndef ARG_UNUSED
#define ARG_UNUSED(x) ((void)(x))
#endif

/* Zephyr IS_ENABLED: 1 when the arg is defined to 1, 0 when undefined. */
#ifndef IS_ENABLED
#define IS_ENABLED(config_macro) Z_IS_ENABLED1(config_macro)
#define Z_IS_ENABLED1(config_macro) Z_IS_ENABLED2(_XXXX##config_macro)
#define _XXXX1 _YYYY,
#define Z_IS_ENABLED2(one_or_two_args) Z_IS_ENABLED3(one_or_two_args 1, 0)
#define Z_IS_ENABLED3(ignore_this, val, ...) val
#endif

#endif /* WOZ_ESP_COMPAT_UTIL_H */
