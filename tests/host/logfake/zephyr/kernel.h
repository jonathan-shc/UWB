/* logfake: minimal <zephyr/kernel.h> for host-building the log facade. */
#ifndef LOGFAKE_ZEPHYR_KERNEL_H
#define LOGFAKE_ZEPHYR_KERNEL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif
#ifndef ARG_UNUSED
#define ARG_UNUSED(x) (void)(x)
#endif

/* snprintk is snprintf with a trimmed format set; host snprintf covers it. */
#define snprintk snprintf

static inline unsigned int irq_lock(void)
{
	return 0u;
}
static inline void irq_unlock(unsigned int key)
{
	(void)key;
}

struct k_timer;
typedef void (*k_timer_expiry_t)(struct k_timer *timer);
struct k_timer {
	k_timer_expiry_t expiry_fn;
	void (*stop_fn)(struct k_timer *timer);
};
#define K_TIMER_DEFINE(name, expiry, stop) struct k_timer name = {(expiry), (stop)}

typedef int64_t k_timeout_t; /* milliseconds in this fake */
#define K_SECONDS(s) ((k_timeout_t)(s) * 1000)

void k_timer_start(struct k_timer *timer, k_timeout_t duration, k_timeout_t period);

#endif /* LOGFAKE_ZEPHYR_KERNEL_H */
