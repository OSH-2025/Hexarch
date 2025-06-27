#ifndef FREERTOS_LOG_H
#define FREERTOS_LOG_H

#include "FreeRTOS.h"
#include "semphr.h"
#include "riscv-virt.h"
#include <stdbool.h>
#include <stdint.h>

/* 简化的日志级别定义 */
typedef enum {
    LOG_LEVEL_NONE = 0,   /* 不输出任何日志 */
    LOG_LEVEL_ERROR,      /* 错误信息 */
    LOG_LEVEL_INFO,       /* 一般信息 */
    LOG_LEVEL_DEBUG,      /* 调试信息 */
} LogLevel_t;

/* 简化的日志系统配置结构体 */
typedef struct {
    LogLevel_t level;         /* 当前日志级别 */
    bool show_timestamp;      /* 是否显示时间戳 */
} LogConfig_t;

/* 日志系统初始化与配置函数 */
bool log_init(LogConfig_t *config);
void log_set_level(LogLevel_t level);
LogLevel_t log_get_level(void);

/* 基础日志函数 - 不使用可变参数，避免依赖复杂格式化库 */
void log_error(const char *tag, const char *message);
void log_info(const char *tag, const char *message);
void log_debug(const char *tag, const char *message);

/* 整数值记录函数 - 避免浮点数处理 */
void log_error_int(const char *tag, const char *message, int value);
void log_info_int(const char *tag, const char *message, int value);
void log_debug_int(const char *tag, const char *message, int value);

/* 十六进制值记录函数 */
void log_error_hex(const char *tag, const char *message, uint32_t value);
void log_info_hex(const char *tag, const char *message, uint32_t value);
void log_debug_hex(const char *tag, const char *message, uint32_t value);

/* 字符串连接记录函数 */
void log_error_str(const char *tag, const char *message, const char *str_value);
void log_info_str(const char *tag, const char *message, const char *str_value);
void log_debug_str(const char *tag, const char *message, const char *str_value);

#endif /* FREERTOS_LOG_H */
