/* logfake: minimal <zephyr/logging/log.h>. */
#ifndef LOGFAKE_ZEPHYR_LOGGING_LOG_H
#define LOGFAKE_ZEPHYR_LOGGING_LOG_H

#include <stdint.h>

#include <zephyr/logging/log_msg.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif

#define LOG_LEVEL_NONE 0u
#define LOG_LEVEL_ERR  1u
#define LOG_LEVEL_WRN  2u
#define LOG_LEVEL_INF  3u
#define LOG_LEVEL_DBG  4u

const char *log_source_name_get(uint32_t domain_id, int16_t source_id);
int16_t log_source_id_get(const char *name);

#endif /* LOGFAKE_ZEPHYR_LOGGING_LOG_H */
