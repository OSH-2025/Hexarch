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

/* 测试各种日志级别和类型的任务 */
void log_test_task(void *pParam) {
    int count = 0;
    
    uart_puts("Starting log test task\n");
    
    while(count < 30) {
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
        // vTaskDelay(500);
        
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
    return ;
}

/* 简单整数转字符串函数 */
static void int_to_string(int num, char *buffer) {
    char tmp[12];
    int i = 0;
    
    /* 特殊情况: 0 */
    if (num == 0) {
        buffer[0] = '0';
        buffer[1] = '\0';
        return;
    }
    
    /* 将数字转换为字符串 (逆序) */
    while (num > 0) {
        tmp[i++] = '0' + (num % 10);
        num /= 10;
    }
    
    /* 反转字符串 */
    int j = 0;
    while (i > 0) {
        buffer[j++] = tmp[--i];
    }
    buffer[j] = '\0';
}

/* 调度器测试任务 - 不依赖硬件GPIO */
void scheduler_test_task(void *pParam) {
    int counter = 0;
    int task_id = (int)pParam;
    char task_name[20] = "Task-";
    char id_str[5];
    
    /* 构造任务名称 */
    int_to_string(task_id, id_str);
    int i = 5; // "Task-" 的长度
    int j = 0;
    while (id_str[j]) {
        task_name[i++] = id_str[j++];
    }
    task_name[i] = '\0';
    
    /* 使用日志API记录任务启动 */
    log_info_str("SCHED", "启动调度器测试任务", task_name);
    
    while(counter < 50) {
        counter++;
        
        /* 使用日志API记录计数 */
        log_info_int(task_name, "循环计数", counter);

        /* 每5次循环，主动放弃CPU以测试调度 */
        if (counter % 5 == 0) {
            log_info(task_name, "主动放弃CPU");
            taskYIELD();
        }
    }
    return ;
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
    xTaskCreate(log_test_task, (const signed char *)"LOG_TEST", 512, NULL, 3, NULL); // 高优先级任务
    
    /* 创建多个调度器测试任务，具有不同的优先级和参数 */
    xTaskCreate(scheduler_test_task, (const signed char *)"SCHED_TEST1", 256, (void *)1, 2, NULL); // 低优先级
    xTaskCreate(scheduler_test_task, (const signed char *)"SCHED_TEST2", 256, (void *)2, 2, NULL); // 中优先级
    
    uart_puts("任务创建完成，开始调度器...\n");
    
    /* 启动FreeRTOS调度器 */
    vTaskStartScheduler();
    
    /* 如果调度器失败，进入安全循环 */
    while(1) {
        ;
    }
}