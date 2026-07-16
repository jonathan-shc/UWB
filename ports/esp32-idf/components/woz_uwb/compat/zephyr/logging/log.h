/* ESP-IDF compat for <zephyr/logging/log.h> — Zephyr LOG_* mapped onto esp_log.
 * Each module's LOG_MODULE_REGISTER/DECLARE(name, ...) fixes a per-file TAG. */
#ifndef WOZ_ESP_COMPAT_LOG_H
#define WOZ_ESP_COMPAT_LOG_H

#include <stdio.h>
#include "esp_log.h"

#define LOG_LEVEL_NONE 0
#define LOG_LEVEL_ERR  1
#define LOG_LEVEL_WRN  2
#define LOG_LEVEL_INF  3
#define LOG_LEVEL_DBG  4

#define LOG_MODULE_REGISTER(name, ...) \
	static const char *const WOZ_LOG_TAG __attribute__((unused)) = #name
#define LOG_MODULE_DECLARE(name, ...) \
	static const char *const WOZ_LOG_TAG __attribute__((unused)) = #name

#define LOG_ERR(...) ESP_LOGE(WOZ_LOG_TAG, __VA_ARGS__)
#define LOG_WRN(...) ESP_LOGW(WOZ_LOG_TAG, __VA_ARGS__)
#define LOG_INF(...) ESP_LOGI(WOZ_LOG_TAG, __VA_ARGS__)
#define LOG_DBG(...) ESP_LOGD(WOZ_LOG_TAG, __VA_ARGS__)
#define LOG_PRINTK(...) printf(__VA_ARGS__)

#define LOG_HEXDUMP_ERR(p, l, s) ESP_LOG_BUFFER_HEX_LEVEL(WOZ_LOG_TAG, (p), (l), ESP_LOG_ERROR)
#define LOG_HEXDUMP_WRN(p, l, s) ESP_LOG_BUFFER_HEX_LEVEL(WOZ_LOG_TAG, (p), (l), ESP_LOG_WARN)
#define LOG_HEXDUMP_INF(p, l, s) ESP_LOG_BUFFER_HEX_LEVEL(WOZ_LOG_TAG, (p), (l), ESP_LOG_INFO)
#define LOG_HEXDUMP_DBG(p, l, s) ESP_LOG_BUFFER_HEX_LEVEL(WOZ_LOG_TAG, (p), (l), ESP_LOG_DEBUG)

#endif /* WOZ_ESP_COMPAT_LOG_H */
