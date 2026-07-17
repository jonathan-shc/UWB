/* ESP-IDF compat for <zephyr/sys/printk.h> — map printk/snprintk onto stdio. */
#ifndef WOZ_ESP_COMPAT_PRINTK_H
#define WOZ_ESP_COMPAT_PRINTK_H

#include <stdio.h>

#define printk(...)    printf(__VA_ARGS__)
#define snprintk(...)  snprintf(__VA_ARGS__)
#define vsnprintk(...) vsnprintf(__VA_ARGS__)

#endif /* WOZ_ESP_COMPAT_PRINTK_H */
