/* SPDX-License-Identifier: ISC */
#include "unlock_gate.h"

#include <stdatomic.h>

static atomic_bool s_allowed = ATOMIC_VAR_INIT(true);

bool unlock_gate_allows_passive(void)
{
	return atomic_load_explicit(&s_allowed, memory_order_relaxed);
}

void unlock_gate_set(bool allowed)
{
	atomic_store_explicit(&s_allowed, allowed, memory_order_relaxed);
}

void unlock_gate_toggle(void)
{
	unlock_gate_set(!unlock_gate_allows_passive());
}
