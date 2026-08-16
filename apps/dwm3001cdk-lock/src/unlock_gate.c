/* SPDX-License-Identifier: ISC */
#include "unlock_gate.h"

#include <zephyr/kernel.h>

static atomic_t s_allowed = ATOMIC_INIT(1);

bool unlock_gate_allows_passive(void)
{
	return atomic_get(&s_allowed) != 0;
}

void unlock_gate_set(bool allowed)
{
	atomic_set(&s_allowed, allowed ? 1 : 0);
}

void unlock_gate_toggle(void)
{
	unlock_gate_set(!unlock_gate_allows_passive());
}
