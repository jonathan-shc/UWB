<!-- generated documentation — edit the source, not this file -->
# `ports/esp32-matter/main/lock_led.h`

Lock status LED color mapping: derives the RGB color for the lock indicator from the
current locked and Aliro-ranging state.

**used by** [`ports/esp32-matter/main/app_driver.cpp`](app_driver.cpp.md), [`ports/esp32-matter/main/lock_led.c`](lock_led.c.md)

## API

### `struct lock_led_rgb lock_led_color(bool locked, bool aliro)`
`ports/esp32-matter/main/lock_led.h:36`

Locked extinguishes the indicator. Unlocked lights blue when the UWB
approach path drove it and green otherwise, so an approach-unlock is
distinguishable from a Home-app tap at a glance. @p aliro is ignored
when @p locked is true.
