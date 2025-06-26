#include "log.h"
#include <string.h>

/* 简单字符输出函数 - 使用最基本的方式直接输出字符 */
static void uart_putc(char c) {
    /* 
     * 实现一个极简的字符输出函数
     * 这里应该调用底层的UART发送函数，具体实现取决于平台
     * 假设有一个UART寄存器地址，我们直接写入字符
     */
    volatile char *uart_data = (volatile char *)0x20201000; /* UART0 数据寄存器地址，请替换为正确的地址 */
    *uart_data = c;
}

/* 简单字符串输出函数 - 不依赖stdio库 */
static void uart_puts(const char *str) {
    while (*str) {
        uart_putc(*str++);
    }
}

/* 字符缓冲区处理辅助函数 */
static void append_to_buffer(char *buffer, int *position, const char *str) {
    while (*str && *position < 255) { /* 保留一位用于NULL终止符 */
        buffer[(*position)++] = *str++;
    }
    buffer[*position] = '\0'; /* 确保字符串以NULL结尾 */
}

/* 整数转字符串函数 - 用于替代sprintf */
static void int_to_str(int num, char *buffer, int base) {
    static const char digits[] = "0123456789ABCDEF";
    char tmp[12]; /* 足够放置32位整数的字符串 */
    int i = 0;
    int sign = 0;
    
    /* 处理负数 */
    if (num < 0 && base == 10) {
        sign = 1;
        num = -num;
    }
    
    /* 特殊情况: 0 */
    if (num == 0) {
        buffer[0] = '0';
        buffer[1] = '\0';
        return;
    }
    
    /* 将数字转换为对应进制的字符串 (逆序) */
    while (num > 0) {
        tmp[i++] = digits[num % base];
        num /= base;
    }
    
    /* 添加负号（如果是负数） */
    if (sign) {
        buffer[0] = '-';
        sign = 1;
    } else {
        sign = 0;
    }
    
    /* 反转字符串 */
    int j = 0;
    while (i > 0) {
        buffer[j + sign] = tmp[--i];
        j++;
    }
    buffer[j + sign] = '\0';
}

/* 十六进制数转字符串 */
static void hex_to_str(uint32_t num, char *buffer) {
    static const char digits[] = "0123456789ABCDEF";
    int i = 0;
    
    /* 添加0x前缀 */
    buffer[i++] = '0';
    buffer[i++] = 'x';
    
    /* 转换过程 */
    bool leading_zero = true;
    for (int shift = 28; shift >= 0; shift -= 4) {
        uint8_t digit = (num >> shift) & 0xF;
        if (digit != 0 || !leading_zero || shift == 0) {
            leading_zero = false;
            buffer[i++] = digits[digit];
        }
    }
    
    buffer[i] = '\0';
}

/* 日志系统配置 */
static LogConfig_t log_config = {
    .level = LOG_LEVEL_INFO,
    .show_timestamp = true
};

/* 互斥量，用于线程安全 */
static xSemaphoreHandle log_mutex = NULL;

/* 简化的日志级别对应的文本 */
static const char *level_strings[] = {
    "NONE", "ERROR", "INFO ", "DEBUG"
};

/* 初始化日志系统 */
bool log_init(LogConfig_t *config) {
    if (log_mutex != NULL) {
        vSemaphoreDelete(log_mutex);
    }
    
    vSemaphoreCreateBinary(log_mutex);
    if (log_mutex == NULL) {
        return false; /* 创建信号量失败 */
    }
    
    if (config != NULL) {
        memcpy(&log_config, config, sizeof(LogConfig_t));
    }
    
    return true;
}

/* 设置日志级别 */
void log_set_level(LogLevel_t level) {
    if (level <= LOG_LEVEL_DEBUG) {
        log_config.level = level;
    }
}

/* 获取当前日志级别 */
LogLevel_t log_get_level(void) {
    return log_config.level;
}

/* 内部通用日志写入函数 */
static void log_write_internal(LogLevel_t level, const char *tag, const char *message) {
    /* 检查日志级别 */
    if (level > log_config.level || level == LOG_LEVEL_NONE) {
        return;
    }
    
    /* 尝试获取互斥量 */
    if (log_mutex != NULL && xSemaphoreTake(log_mutex, portMAX_DELAY) != pdTRUE) {
        return;  /* 获取互斥量失败 */
    }
    
    /* 使用静态缓冲区而不是动态分配，避免内存碎片 */
    static char buffer[256];
    int pos = 0;
    
    /* 添加时间戳 */
    if (log_config.show_timestamp) {
        portTickType ticks = xTaskGetTickCount();
        char tick_str[12];
        int_to_str(ticks, tick_str, 10);
        
        append_to_buffer(buffer, &pos, "[");
        append_to_buffer(buffer, &pos, tick_str);
        append_to_buffer(buffer, &pos, "] ");
    }
    
    /* 添加日志级别和标签 */
    append_to_buffer(buffer, &pos, level_strings[level]);
    append_to_buffer(buffer, &pos, " (");
    append_to_buffer(buffer, &pos, tag);
    append_to_buffer(buffer, &pos, "): ");
    
    /* 添加日志消息 */
    append_to_buffer(buffer, &pos, message);
    
    /* 添加换行符 */
    append_to_buffer(buffer, &pos, "\r\n");
    
    /* 输出日志 */
    uart_puts(buffer);
    
    /* 释放互斥量 */
    if (log_mutex != NULL) {
        xSemaphoreGive(log_mutex);
    }
}

/* 基础日志函数实现 */
void log_error(const char *tag, const char *message) {
    log_write_internal(LOG_LEVEL_ERROR, tag, message);
}

void log_info(const char *tag, const char *message) {
    log_write_internal(LOG_LEVEL_INFO, tag, message);
}

void log_debug(const char *tag, const char *message) {
    log_write_internal(LOG_LEVEL_DEBUG, tag, message);
}

/* 整数值日志函数实现 */
void log_error_int(const char *tag, const char *message, int value) {
    char buffer[256];
    int pos = 0;
    
    append_to_buffer(buffer, &pos, message);
    append_to_buffer(buffer, &pos, ": ");
    
    char num_str[12];
    int_to_str(value, num_str, 10);
    append_to_buffer(buffer, &pos, num_str);
    
    log_error(tag, buffer);
}

void log_info_int(const char *tag, const char *message, int value) {
    char buffer[256];
    int pos = 0;
    
    append_to_buffer(buffer, &pos, message);
    append_to_buffer(buffer, &pos, ": ");
    
    char num_str[12];
    int_to_str(value, num_str, 10);
    append_to_buffer(buffer, &pos, num_str);
    
    log_info(tag, buffer);
}

void log_debug_int(const char *tag, const char *message, int value) {
    char buffer[256];
    int pos = 0;
    
    append_to_buffer(buffer, &pos, message);
    append_to_buffer(buffer, &pos, ": ");
    
    char num_str[12];
    int_to_str(value, num_str, 10);
    append_to_buffer(buffer, &pos, num_str);
    
    log_debug(tag, buffer);
}

/* 十六进制值日志函数实现 */
void log_error_hex(const char *tag, const char *message, uint32_t value) {
    char buffer[256];
    int pos = 0;
    
    append_to_buffer(buffer, &pos, message);
    append_to_buffer(buffer, &pos, ": ");
    
    char hex_str[12];
    hex_to_str(value, hex_str);
    append_to_buffer(buffer, &pos, hex_str);
    
    log_error(tag, buffer);
}

void log_info_hex(const char *tag, const char *message, uint32_t value) {
    char buffer[256];
    int pos = 0;
    
    append_to_buffer(buffer, &pos, message);
    append_to_buffer(buffer, &pos, ": ");
    
    char hex_str[12];
    hex_to_str(value, hex_str);
    append_to_buffer(buffer, &pos, hex_str);
    
    log_info(tag, buffer);
}

void log_debug_hex(const char *tag, const char *message, uint32_t value) {
    char buffer[256];
    int pos = 0;
    
    append_to_buffer(buffer, &pos, message);
    append_to_buffer(buffer, &pos, ": ");
    
    char hex_str[12];
    hex_to_str(value, hex_str);
    append_to_buffer(buffer, &pos, hex_str);
    
    log_debug(tag, buffer);
}

/* 字符串连接日志函数实现 */
void log_error_str(const char *tag, const char *message, const char *str_value) {
    char buffer[256];
    int pos = 0;
    
    append_to_buffer(buffer, &pos, message);
    append_to_buffer(buffer, &pos, ": ");
    append_to_buffer(buffer, &pos, str_value);
    
    log_error(tag, buffer);
}

void log_info_str(const char *tag, const char *message, const char *str_value) {
    char buffer[256];
    int pos = 0;
    
    append_to_buffer(buffer, &pos, message);
    append_to_buffer(buffer, &pos, ": ");
    append_to_buffer(buffer, &pos, str_value);
    
    log_info(tag, buffer);
}

void log_debug_str(const char *tag, const char *message, const char *str_value) {
    char buffer[256];
    int pos = 0;
    
    append_to_buffer(buffer, &pos, message);
    append_to_buffer(buffer, &pos, ": ");
    append_to_buffer(buffer, &pos, str_value);
    
    log_debug(tag, buffer);
}
