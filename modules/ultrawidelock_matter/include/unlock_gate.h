/* SPDX-License-Identifier: ISC */
#pragma once
#include <stdbool.h>

/* Owner-controlled gate for passive UWB approach unlocks. */
bool unlock_gate_allows_passive(void);
void unlock_gate_set(bool allowed);
void unlock_gate_toggle(void);
