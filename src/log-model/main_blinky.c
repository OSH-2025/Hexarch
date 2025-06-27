/*
 * FreeRTOS V202212.00
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
 */


// main_blinky.c中的日志测试函数是从树莓派版本的main.c改过来的


/* FreeRTOS kernel includes. */
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

#include <stdio.h>

#include "riscv-virt.h"
#include "ns16550.h"
#include "log.h"

#define UART0_BASE  0x101f1000
#define UART0_DR    (*(volatile unsigned int*)(UART0_BASE + 0x00))
#define UART0_FR    (*(volatile unsigned int*)(UART0_BASE + 0x18))

/* Priorities used by the tasks. */
#define mainQUEUE_RECEIVE_TASK_PRIORITY    ( tskIDLE_PRIORITY + 2 )
#define mainQUEUE_SEND_TASK_PRIORITY       ( tskIDLE_PRIORITY + 1 )

/* The rate at which data is sent to the queue.  The 200ms value is converted
 * to ticks using the pdMS_TO_TICKS() macro. */
#define mainQUEUE_SEND_FREQUENCY_MS        pdMS_TO_TICKS( 1000 )

/* The maximum number items the queue can hold.  The priority of the receiving
 * task is above the priority of the sending task, so the receiving task will
 * preempt the sending task and remove the queue items each time the sending task
 * writes to the queue.  Therefore the queue will never have more than one item in
 * it at any time, and even with a queue length of 1, the sending task will never
 * find the queue full. */
#define mainQUEUE_LENGTH                   ( 1 )

/*-----------------------------------------------------------*/

/* The queue used by both tasks. */
static QueueHandle_t xQueue = NULL;

/*-----------------------------------------------------------*/

static void prvQueueSendTask( void * pvParameters )
{
    TickType_t xNextWakeTime;
    const unsigned long ulValueToSend = 100UL;
    const char * const pcMessage1 = "Transfer1";
    const char * const pcMessage2 = "Transfer2";
    int f = 1;

    /* Remove compiler warning about unused parameter. */
    ( void ) pvParameters;

    /* Initialise xNextWakeTime - this only needs to be done once. */
    xNextWakeTime = xTaskGetTickCount();

    for( ; ; )
    {
        char buf[ 40 ];

        sprintf( buf, "%d: %s: %s", xGetCoreID(),
                 pcTaskGetName( xTaskGetCurrentTaskHandle() ),
                 ( f ) ? pcMessage1 : pcMessage2 );
        vSendString( buf );
        f = !f;

        /* Place this task in the blocked state until it is time to run again. */
        vTaskDelayUntil( &xNextWakeTime, mainQUEUE_SEND_FREQUENCY_MS );

        /* Send to the queue - causing the queue receive task to unblock and
         * toggle the LED.  0 is used as the block time so the sending operation
         * will not block - it shouldn't need to block as the queue should always
         * be empty at this point in the code. */
        xQueueSend( xQueue, &ulValueToSend, 0U );
    }
}

/*-----------------------------------------------------------*/

static void prvQueueReceiveTask( void * pvParameters )
{
    unsigned long ulReceivedValue;
    const unsigned long ulExpectedValue = 100UL;
    const char * const pcMessage1 = "Blink1";
    const char * const pcMessage2 = "Blink2";
    const char * const pcFailMessage = "Unexpected value received\r\n";
    int f = 1;

    /* Remove compiler warning about unused parameter. */
    ( void ) pvParameters;

    for( ; ; )
    {
        char buf[ 40 ];

        /* Wait until something arrives in the queue - this task will block
         * indefinitely provided INCLUDE_vTaskSuspend is set to 1 in
         * FreeRTOSConfig.h. */
        xQueueReceive( xQueue, &ulReceivedValue, portMAX_DELAY );

        /*  To get here something must have been received from the queue, but
         * is it the expected value?  If it is, toggle the LED. */
        if( ulReceivedValue == ulExpectedValue )
        {
            sprintf( buf, "%d: %s: %s", xGetCoreID(),
                     pcTaskGetName( xTaskGetCurrentTaskHandle() ),
                     ( f ) ? pcMessage1 : pcMessage2 );
            vSendString( buf );
            f = !f;

            ulReceivedValue = 0U;
        }
        else
        {
            vSendString( pcFailMessage );
        }
    }
}

/*-----------------------------------------------------------*/

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
    return;
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
        
        /* 使用简单循环来延迟，不依赖于vTaskDelay */
        for(volatile int i = 0; i < 100000; i++) {
            /* 空循环延迟 */
        }
        
        /* 每5次循环，主动放弃CPU以测试调度 */
        if (counter % 5 == 0) {
            log_info(task_name, "主动放弃CPU");
            taskYIELD();
        }
        // vTaskDelay(100);
    }
    return;
}

/*-----------------------------------------------------------*/

int main_blinky( void )
{
    vSendString( "Hello FreeRTOS!" );

    log_info_str("SCHED", "启动调度器测试任务", "task_name");

    /* 初始化日志系统 */
    LogConfig_t log_config = {
        .level = LOG_LEVEL_INFO,
        .show_timestamp = true
    };
    
    if (log_init(&log_config)) {
        // uart_puts("日志系统初始化成功\n");
        vSendString("日志系统初始化成功\n");
    } else {
        // uart_puts("日志系统初始化失败\n");
        vSendString("日志系统初始化失败\n");
    }
    
    /* Create the queue. */
    xQueue = xQueueCreate( mainQUEUE_LENGTH, sizeof( uint32_t ) );

    if( xQueue != NULL )
    {
        /* Start the two tasks as described in the comments at the top of this
         * file. */
        xTaskCreate( prvQueueReceiveTask, "Rx", configMINIMAL_STACK_SIZE * 2U, NULL,
                     mainQUEUE_RECEIVE_TASK_PRIORITY, NULL );
        xTaskCreate( prvQueueSendTask, "Tx", configMINIMAL_STACK_SIZE * 2U, NULL,
                     mainQUEUE_SEND_TASK_PRIORITY, NULL );

        /* 创建日志测试任务 */
        xTaskCreate(log_test_task, "LOG_TEST", 512, NULL, 3, NULL);
    
        /* 创建多个调度器测试任务，具有不同的优先级和参数 */
        xTaskCreate(scheduler_test_task, "SCHED_TEST1", 256, (void *)1, 0, NULL); // 低优先级
        xTaskCreate(scheduler_test_task, "SCHED_TEST2", 256, (void *)2, 0, NULL); // 中优先级
    }

    // uart_puts("任务创建完成，开始调度器...\n");
    vSendString("任务创建完成，开始调度器...\n");

    vTaskStartScheduler();

    return 0;
}
