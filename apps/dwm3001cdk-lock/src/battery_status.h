/* SPDX-License-Identifier: ISC */
#pragma once
#include <stdint.h>

/* Return a coarse battery state estimate as 0..100 percent. */
uint8_t battery_status_percent(void);

/* Last measured nRF VDD in millivolts, or 0 when no valid sample exists yet. */
uint16_t battery_status_vdd_mv(void);
