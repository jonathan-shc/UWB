/* SPDX-License-Identifier: ISC */
#pragma once
#include <stdbool.h>

/* Owner-controlled gate for passive UWB approach unlocks. */
bool unlock_gate_allows_passive(void);
void unlock_gate_set(bool allowed);
void unlock_gate_toggle(void);

/*
 * Session latch for passive approach unlocks.
 *
 * Once a session is blocked by the owner gate it stays blocked until the next
 * credential/approach session begins. This prevents repeated grant attempts and
 * deliberately avoids an already-present phone unlocking immediately when the
 * owner flips the switch back on.
 */
void unlock_gate_session_reset(void);
bool unlock_gate_session_blocked(void);
void unlock_gate_session_block(void);
