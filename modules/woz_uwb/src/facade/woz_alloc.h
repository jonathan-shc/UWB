// Memory allocation and timing facade: qmalloc, qcalloc, qfree wrap the platform heap;
// qrtc_get_us returns monotonic microseconds since boot.
/*
 * Copyright (c) 2026 asxeem
 * SPDX-License-Identifier: ISC
 * woz_alloc.h - thin heap + monotonic-clock wrappers over woz_port.h.
 */
#ifndef WOZ_ALLOC_H
#define WOZ_ALLOC_H

#include <stddef.h>
#include <stdint.h>

#include "woz_port.h"

// Allocate size bytes; wrapper around woz_malloc.
static inline void *qmalloc(size_t size)
{
	return woz_malloc(size);
}

// Allocate and zero-initialize nb_items elements of item_size bytes each; wrapper around woz_calloc.
static inline void *qcalloc(size_t nb_items, size_t item_size)
{
	return woz_calloc(nb_items, item_size);
}

// Deallocate memory previously allocated by qmalloc or qcalloc; wrapper around woz_free.
static inline void qfree(void *ptr)
{
	woz_free(ptr);
}

/** Monotonic microseconds since boot. */
static inline int64_t qrtc_get_us(void)
{
	return woz_uptime_us();
}

#endif /* WOZ_ALLOC_H */
