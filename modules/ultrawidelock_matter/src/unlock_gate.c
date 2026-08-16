/* SPDX-License-Identifier: ISC */
#include "unlock_gate.h"

#include <stdatomic.h>

static atomic_bool s_allowed = ATOMIC_VAR_INIT(true);
static atomic_bool s_session_blocked = ATOMIC_VAR_INIT(false);

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

void unlock_gate_session_reset(void)
{
	atomic_store_explicit(&s_session_blocked, false, memory_order_relaxed);
}

bool unlock_gate_session_blocked(void)
{
	return atomic_load_explicit(&s_session_blocked, memory_order_relaxed);
}

void unlock_gate_session_block(void)
{
	atomic_store_explicit(&s_session_blocked, true, memory_order_relaxed);
}
