/*
 * 简单的日志系统测试程序
 * 用于测试FreeRTOS下日志系统的功能
 */

#include <FreeRTOS.h>
#include <task.h>
#include "Drivers/gpio.h"
#include "log.h"

#define UART0_BASE  0x101f1000
#define UART0_DR    (*(volatile unsigned int*)(UART0_BASE + 0x00))
#define UART0_FR    (*(volatile unsigned int*)(UART0_BASE + 0x18))

/* 基础UART输出函数 */
void uart_putc(char c) {
    while (UART0_FR & (1 << 5)) ; // 等待发送FIFO非满
    UART0_DR = c;
}

void uart_puts(const char* s) {
    while (*s) {
        uart_putc(*s++);
    }
}

/* 简单的LED闪烁任务，用于验证FreeRTOS任务调度 */
void led_blink_task(void *pParam) {
    int counter = 0;
    
    while(1) {
        counter++;
        
        /* 每秒记录一次日志 */
        if (counter % 10 == 0) {
            log_info("LED", "LED闪烁次数");
            log_info_int("LED", "计数值", counter);
            uart_puts("LED blink count: ");
            
            /* 简单的数字输出 */
            char buf[16];
            int i = 0, temp = counter;
            do {
                buf[i++] = '0' + (temp % 10);
                temp /= 10;
            } while(temp > 0);
            
            while(i > 0) {
                uart_putc(buf[--i]);
            }
            uart_puts("\n");
        }
        
        /* LED闪烁 */
        SetGpio(16, 1);
        vTaskDelay(50);
        SetGpio(16, 0);
        vTaskDelay(50);
    }
}

/* 测试各种日志级别和类型的任务 */
void log_test_task(void *pParam) {
    int count = 0;
    
    uart_puts("Starting log test task\n");
    
    while(1) {
        count++;
        
        /* 基本日志测试 */
        log_error("TEST", "这是一条错误日志");
        log_info("TEST", "这是一条信息日志");
        log_debug("TEST", "这是一条调试日志");
        
        /* 数值日志测试 */
        log_error_int("TEST", "错误日志-整数值", count);
        log_info_int("TEST", "信息日志-整数值", count * 10);
        log_debug_int("TEST", "调试日志-整数值", count * 100);
        
        /* 十六进制值测试 */
        log_error_hex("TEST", "错误日志-十六进制值", count);
        log_info_hex("TEST", "信息日志-十六进制值", 0xABCD);
        log_debug_hex("TEST", "调试日志-十六进制值", 0x12345678);
        
        /* 字符串值测试 */
        log_error_str("TEST", "错误日志-字符串值", "测试错误字符串");
        log_info_str("TEST", "信息日志-字符串值", "测试信息字符串");
        log_debug_str("TEST", "调试日志-字符串值", "测试调试字符串");
        
        /* 直接使用uart输出进行验证 */
        uart_puts("\n==================\n");
        uart_puts("Completed log test cycle ");
        
        /* 简单的数字输出 */
        char buf[16];
        int i = 0, temp = count;
        do {
            buf[i++] = '0' + (temp % 10);
            temp /= 10;
        } while(temp > 0);
        
        while(i > 0) {
            uart_putc(buf[--i]);
        }
        uart_puts("\n==================\n\n");
        
        /* 每次测试间隔5秒 */
        vTaskDelay(500);
        
        /* 每3次循环切换一下日志级别 */
        if (count % 3 == 0) {
            LogLevel_t current_level = log_get_level();
            
            /* 轮换日志级别: ERROR -> INFO -> DEBUG -> ERROR ... */
            if (current_level == LOG_LEVEL_ERROR) {
                log_set_level(LOG_LEVEL_INFO);
                uart_puts("Log level changed to INFO\n");
            } else if (current_level == LOG_LEVEL_INFO) {
                log_set_level(LOG_LEVEL_DEBUG);
                uart_puts("Log level changed to DEBUG\n");
            } else {
                log_set_level(LOG_LEVEL_ERROR);
                uart_puts("Log level changed to ERROR\n");
            }
        }
    }
}

/**
 * 系统入口函数
 */
void main(void) {
    /* 初始化UART（在QEMU中通常不需要额外配置） */
    uart_puts("\n\n=== 日志系统测试程序启动 ===\n\n");
    
    /* 初始化LED GPIO */
    SetGpioFunction(16, 1);
    
    /* 初始化日志系统 */
    LogConfig_t log_config = {
        .level = LOG_LEVEL_INFO,
        .show_timestamp = true
    };
    
    if (log_init(&log_config)) {
        uart_puts("日志系统初始化成功\n");
    } else {
        uart_puts("日志系统初始化失败\n");
    }
    
    /* 创建测试任务 */
    xTaskCreate(led_blink_task, (const signed char *)"LED_BLINK", 128, NULL, 1, NULL);
    xTaskCreate(log_test_task, (const signed char *)"LOG_TEST", 512, NULL, 2, NULL);
    
    uart_puts("任务创建完成，开始调度器...\n");
    
    /* 启动FreeRTOS调度器 */
    vTaskStartScheduler();
    
    /* 如果调度器失败，进入安全循环 */
    while(1) {
        ;
    }
}
