/*
 * Copyright (c) 2026 asxeem
 * SPDX-License-Identifier: ISC
 */
#include "lock_led.h"

struct lock_led_rgb lock_led_color(bool locked, bool aliro)
{
	struct lock_led_rgb c = { 0, 0, 0 };

	if (locked) {
		return c;
	}
	if (aliro) {
		c.b = LOCK_LED_BRIGHTNESS;
	} else {
		c.g = LOCK_LED_BRIGHTNESS;
	}
	return c;
}
