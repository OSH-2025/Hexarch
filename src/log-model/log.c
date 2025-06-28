#include "log.h"
#include <string.h>

/* Simple character output function - use the most basic way to output characters directly */
static void uart_putc(char c) {
    /* 
     * Implement a minimalist character output function
     * This should call the underlying UART send function, specific implementation depends on platform
     * Use the same UART address as in main.c
     */
    volatile unsigned int *uart_data = (volatile unsigned int *)0x101f1000; /* Consistent with UART0_DR address in main.c */
    *uart_data = c;
}

/* Simple string output function - does not depend on stdio library */
static void uart_puts(const char *str) {
    while (*str) {
        uart_putc(*str++);
    }
}

/* Character buffer processing helper function */
static void append_to_buffer(char *buffer, int *position, const char *str) {
    while (*str && *position < 255) { /* Reserve one bit for NULL terminator */
        buffer[(*position)++] = *str++;
    }
    buffer[*position] = '\0'; /* Ensure string ends with NULL */
}

/* Integer to string function - used to replace sprintf */
static void int_to_str(int num, char *buffer, int base) {
    static const char digits[] = "0123456789ABCDEF";
    char tmp[12]; /* Enough to hold 32-bit integer string */
    int i = 0;
    int sign = 0;
    
    /* Handle negative numbers */
    if (num < 0 && base == 10) {
        sign = 1;
        num = -num;
    }
    
    /* Special case: 0 */
    if (num == 0) {
        buffer[0] = '0';
        buffer[1] = '\0';
        return;
    }
    
    /* Convert number to corresponding base string (reverse order) */
    while (num > 0) {
        tmp[i++] = digits[num % base];
        num /= base;
    }
    
    /* Add negative sign (if negative) */
    if (sign) {
        buffer[0] = '-';
        sign = 1;
    } else {
        sign = 0;
    }
    
    /* Reverse string */
    int j = 0;
    while (i > 0) {
        buffer[j + sign] = tmp[--i];
        j++;
    }
    buffer[j + sign] = '\0';
}

/* Hexadecimal number to string */
static void hex_to_str(uint32_t num, char *buffer) {
    static const char digits[] = "0123456789ABCDEF";
    int i = 0;
    
    /* Add 0x prefix */
    buffer[i++] = '0';
    buffer[i++] = 'x';
    
    /* Conversion process */
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

/* Log system configuration */
static LogConfig_t log_config = {
    .level = LOG_LEVEL_INFO,
    .show_timestamp = true
};

/* Mutex for thread safety */
static xSemaphoreHandle log_mutex = NULL;

/* Simplified log level corresponding text */
static const char *level_strings[] = {
    "NONE", "ERROR", "INFO ", "DEBUG"
};

/* Initialize log system */
bool log_init(LogConfig_t *config) {
    if (log_mutex != NULL) {
        vSemaphoreDelete(log_mutex);
    }
    
    vSemaphoreCreateBinary(log_mutex);
    if (log_mutex == NULL) {
        return false; /* Failed to create semaphore */
    }
    
    if (config != NULL) {
        memcpy(&log_config, config, sizeof(LogConfig_t));
    }
    
    return true;
}

/* Set log level */
void log_set_level(LogLevel_t level) {
    if (level <= LOG_LEVEL_DEBUG) {
        log_config.level = level;
    }
}

/* Get current log level */
LogLevel_t log_get_level(void) {
    return log_config.level;
}

/* Internal common log write function */
/* Output example
 * log_write_internal(LOG_LEVEL_ERROR, "TAG", "An error occurred");
 * Output: [12345] ERROR (TAG): An error occurred
 */
static void log_write_internal(LogLevel_t level, const char *tag, const char *message) {
    /* Check log level */
    if (level > log_config.level || level == LOG_LEVEL_NONE) {
        return;
    }
    
    /* Try to acquire mutex */
    if (log_mutex != NULL && xSemaphoreTake(log_mutex, portMAX_DELAY) != pdTRUE) {
        return;  /* Failed to acquire mutex */
    }
    
    /* Use static buffer instead of dynamic allocation to avoid memory fragmentation */
    static char buffer[256];
    int pos = 0;
    
    /* Add timestamp */
    if (log_config.show_timestamp) {
        portTickType ticks = xTaskGetTickCount();
        char tick_str[12];
        int_to_str(ticks, tick_str, 10);
        
        append_to_buffer(buffer, &pos, "[");
        append_to_buffer(buffer, &pos, tick_str);
        append_to_buffer(buffer, &pos, "] ");
    }
    
    /* Add log level and tag */
    append_to_buffer(buffer, &pos, level_strings[level]);
    append_to_buffer(buffer, &pos, " (");
    append_to_buffer(buffer, &pos, tag);
    append_to_buffer(buffer, &pos, "): ");
    
    /* Add log message */
    append_to_buffer(buffer, &pos, message);
    
    /* Add newline */
    append_to_buffer(buffer, &pos, "\r\n");
    
    /* Output log */
    uart_puts(buffer);
    
    /* Release mutex */
    if (log_mutex != NULL) {
        xSemaphoreGive(log_mutex);
    }
}

/* Basic log function implementation */
void log_error(const char *tag, const char *message) {
    log_write_internal(LOG_LEVEL_ERROR, tag, message);
}

void log_info(const char *tag, const char *message) {
    log_write_internal(LOG_LEVEL_INFO, tag, message);
}

void log_debug(const char *tag, const char *message) {
    log_write_internal(LOG_LEVEL_DEBUG, tag, message);
}

/* Integer value log function implementation */
/* Output example
 * log_error_int("TAG", "Error occurred", -42);
 * Output: [12345] ERROR (TAG): Error occurred: -42
 */
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

/* Output example
 * log_info_int("TAG", "Value is", 123);
 * Output: [12345] INFO (TAG): Value is: 123
 */
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

/* Output example
 * log_debug_int("TAG", "Debug value", 42);
 * Output: [12345] DEBUG (TAG): Debug value: 42
 */
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

/* Hexadecimal value log function implementation */
/* Output example
 * log_error_hex("TAG", "Error code", 0x1A2B3C4D);
 * Output: [12345] ERROR (TAG): Error code: 0x1A2B3C4D
 */ 
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

/* Output example
 * log_info_hex("TAG", "Hex value", 0x12345678);
 * Output: [12345] INFO (TAG): Hex value: 0x12345678
 */
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

/* Output example
 * log_debug_hex("TAG", "Debug hex", 0xABCDEF01);
 * Output: [12345] DEBUG (TAG): Debug hex: 0xABCDEF01
 */
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

/* String concatenation log function implementation */
/* Output example
 * log_error_str("TAG", "Error message", "Something went wrong");
 * Output: [12345] ERROR (TAG): Error message: Something went wrong
 */
void log_error_str(const char *tag, const char *message, const char *str_value) {
    char buffer[256];
    int pos = 0;
    
    append_to_buffer(buffer, &pos, message);
    append_to_buffer(buffer, &pos, ": ");
    append_to_buffer(buffer, &pos, str_value);
    
    log_error(tag, buffer);
}

/* Output example
 * log_info_str("TAG", "Info message", "All systems operational");
 * Output: [12345] INFO (TAG): Info message: All systems operational
 */
void log_info_str(const char *tag, const char *message, const char *str_value) {
    char buffer[256];
    int pos = 0;
    
    append_to_buffer(buffer, &pos, message);
    append_to_buffer(buffer, &pos, ": ");
    append_to_buffer(buffer, &pos, str_value);
    
    log_info(tag, buffer);
}

/* Output example
 * log_debug_str("TAG", "Debug message", "Debugging in progress");
 * Output: [12345] DEBUG (TAG): Debug message: Debugging in progress
 */
void log_debug_str(const char *tag, const char *message, const char *str_value) {
    char buffer[256];
    int pos = 0;
    
    append_to_buffer(buffer, &pos, message);
    append_to_buffer(buffer, &pos, ": ");
    append_to_buffer(buffer, &pos, str_value);
    
    log_debug(tag, buffer);
}
