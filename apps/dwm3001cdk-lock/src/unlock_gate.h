/* SPDX-License-Identifier: ISC */
#pragma once
#include <stdbool.h>

bool unlock_gate_allows_passive(void);
void unlock_gate_set(bool allowed);
void unlock_gate_toggle(void);
