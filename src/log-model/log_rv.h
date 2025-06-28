#ifndef FREERTOS_LOG_H
#define FREERTOS_LOG_H

#include "FreeRTOS.h"

/* Check if log module is enabled */
#if (configUSE_LOG_MODULE == 1)

#include "semphr.h"
#include "riscv-virt.h"
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

#else /* configUSE_LOG_MODULE == 0 */

/* When log module is disabled, provide empty macro definitions */
typedef enum {
    LOG_LEVEL_NONE = 0,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG,
} LogLevel_t;

typedef struct {
    LogLevel_t level;
    int show_timestamp;  /* Use int instead of bool to avoid dependencies */
} LogConfig_t;

/* Empty macro definitions - will be optimized away at compile time */
#define log_init(config)                    (1)
#define log_set_level(level)                do { } while(0)
#define log_get_level()                     (LOG_LEVEL_NONE)

#define log_error(tag, message)             do { } while(0)
#define log_info(tag, message)              do { } while(0)
#define log_debug(tag, message)             do { } while(0)

#define log_error_int(tag, message, value)  do { } while(0)
#define log_info_int(tag, message, value)   do { } while(0)
#define log_debug_int(tag, message, value)  do { } while(0)

#define log_error_hex(tag, message, value)  do { } while(0)
#define log_info_hex(tag, message, value)   do { } while(0)
#define log_debug_hex(tag, message, value)  do { } while(0)

#define log_error_str(tag, message, str)    do { } while(0)
#define log_info_str(tag, message, str)     do { } while(0)
#define log_debug_str(tag, message, str)    do { } while(0)

#endif /* configUSE_LOG_MODULE */

#endif /* FREERTOS_LOG_H */
