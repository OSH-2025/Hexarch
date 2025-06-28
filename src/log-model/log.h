#ifndef FREERTOS_LOG_H
#define FREERTOS_LOG_H

#include "FreeRTOS.h"
#include "semphr.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    LOG_LEVEL_NONE = 0,   /* No log output */
    LOG_LEVEL_ERROR,      /* Error messages */
    LOG_LEVEL_INFO,       /* General information */
    LOG_LEVEL_DEBUG,      /* Debug information */
} LogLevel_t;

typedef struct {
    LogLevel_t level;         /* Current log level */
    bool show_timestamp;      /* Whether to show timestamp */
} LogConfig_t;

/* Log system initialization and configuration functions */
bool log_init(LogConfig_t *config);
void log_set_level(LogLevel_t level);
LogLevel_t log_get_level(void);

/* Basic log functions */
void log_error(const char *tag, const char *message);
void log_info(const char *tag, const char *message);
void log_debug(const char *tag, const char *message);

/* Integer value logging functions */
void log_error_int(const char *tag, const char *message, int value);
void log_info_int(const char *tag, const char *message, int value);
void log_debug_int(const char *tag, const char *message, int value);

/* Hexadecimal value logging functions */
void log_error_hex(const char *tag, const char *message, uint32_t value);
void log_info_hex(const char *tag, const char *message, uint32_t value);
void log_debug_hex(const char *tag, const char *message, uint32_t value);

/* String concatenation logging functions */
void log_error_str(const char *tag, const char *message, const char *str_value);
void log_info_str(const char *tag, const char *message, const char *str_value);
void log_debug_str(const char *tag, const char *message, const char *str_value);

#endif /* FREERTOS_LOG_H */
