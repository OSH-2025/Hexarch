/*
 * 条件编译日志模块使用说明：
 * 
 * 1. 启用日志模块：在 FreeRTOSConfig.h 中设置 configUSE_LOG_MODULE = 1
 *    或者使用 make ENABLE_LOG=1 编译
 *    - 会创建 scheduler_test_task，使用完整的日志功能
 *    - 日志函数正常工作，提供详细的调试信息
 * 
 * 2. 禁用日志模块：在 FreeRTOSConfig.h 中设置 configUSE_LOG_MODULE = 0  
 *    或者使用 make ENABLE_LOG=0 编译
 *    - 会创建 simple_demo_task，使用简单的串口输出
 *    - 日志函数调用被优化掉，节省资源
 * 
 * 编译命令示例：
 * make clean && make ENABLE_LOG=1  # 启用日志版本
 * make clean && make ENABLE_LOG=0  # 禁用日志版本
 */

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

/* FreeRTOS kernel includes. */
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

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

/* 全局计数器，用于记录日志 */
static int counter = 0;

/*-----------------------------------------------------------*/

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
    int count = 0;
    int task_id = (int)pParam;
    char task_name[20] = "SCHED-";
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
    
    while(count < 20) {
        count++;
        
        /* 使用日志API记录计数 */
        log_info_int(task_name, "循环计数", count);
        
        /* 每5次循环，主动放弃CPU以测试调度 */
        if (count % 5 == 0) {
            log_info(task_name, "主动放弃CPU");
            taskYIELD();
        }
        
        /* 添加延时，避免任务过于频繁执行 */
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    
    /* 任务完成后记录并删除自己 */
    log_info_str("SCHED", "任务完成，即将删除", task_name);
    vTaskDelete(NULL);
}

/*-----------------------------------------------------------*/

static void log_send_task( void * pvParameters )
{   
    TickType_t xNextWakeTime;
    const unsigned long ulValueToSend = 100UL;

    // static int counter = 0;

    int task_id = (int)pvParameters;
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
    
    /* Remove compiler warning about unused parameter. */
    ( void ) pvParameters;

    /* Initialise xNextWakeTime - this only needs to be done once. */
    xNextWakeTime = xTaskGetTickCount();

    for( ; ; )
    {
        /* 使用日志API记录计数 */
        log_info_int(task_name, "循环计数", counter);

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

static void log_recv_task( void * pvParameters )
{
    unsigned long ulReceivedValue;
    const unsigned long ulExpectedValue = 100UL;

    int task_id = (int)pvParameters;
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
    
    /* Remove compiler warning about unused parameter. */
    ( void ) pvParameters;

    for( ; ; )
    {
        /* Wait until something arrives in the queue - this task will block
         * indefinitely provided INCLUDE_vTaskSuspend is set to 1 in
         * FreeRTOSConfig.h. */
        xQueueReceive( xQueue, &ulReceivedValue, portMAX_DELAY );

        /*  To get here something must have been received from the queue, but
         * is it the expected value?  If it is, toggle the LED. */
        if( ulReceivedValue == ulExpectedValue )
        {
            /* 使用日志API记录计数 */
            log_info_int(task_name, "循环计数", counter);

            counter++;

            ulReceivedValue = 0U;
        }
        else
        {
            vSendString( "Unexpected value received\r\n" );
        }
    }
}

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

int main_blinky( void )
{
    vSendString("\nHello FreeRTOS!\n");

#if (configUSE_LOG_MODULE == 1)
    log_info_str("SCHED", "启动调度器测试任务", "task_name");

    /* 初始化日志系统 */
    LogConfig_t log_config = {
        .level = LOG_LEVEL_INFO,
        .show_timestamp = 1  
    };
    
    if (log_init(&log_config)) {
        vSendString("日志系统初始化成功\n");
    } else {
        vSendString("日志系统初始化失败\n");
    }
#else
    vSendString("日志模块已禁用\n");
#endif
    
    /* Create the queue. */
    xQueue = xQueueCreate( mainQUEUE_LENGTH, sizeof( uint32_t ) );

    if( xQueue != NULL )
    {
#if (configUSE_LOG_MODULE == 1)
        /* 创建使用日志的生产者和消费者任务 */
        xTaskCreate( log_send_task, "Rx", configMINIMAL_STACK_SIZE * 2U, (void *)1,
                     mainQUEUE_SEND_TASK_PRIORITY, NULL );
        xTaskCreate( log_recv_task, "Tx", configMINIMAL_STACK_SIZE * 2U, (void *)2,
                     mainQUEUE_RECEIVE_TASK_PRIORITY, NULL );
        /* 创建使用日志的调度器测试任务 */
        xTaskCreate( scheduler_test_task, "SCHED_TEST1", configMINIMAL_STACK_SIZE * 2U, (void *)1, 
                    2, NULL);
        xTaskCreate( scheduler_test_task, "SCHED_TEST2", configMINIMAL_STACK_SIZE * 2U, (void *)2, 
                    2, NULL);
#else
        /* 创建不使用日志的生产者和消费者任务 */
        xTaskCreate( prvQueueSendTask, "Rx", configMINIMAL_STACK_SIZE * 2U, (void *)1, 
                    mainQUEUE_SEND_TASK_PRIORITY, NULL);
        xTaskCreate( prvQueueReceiveTask, "Tx", configMINIMAL_STACK_SIZE * 2U, (void *)2, 
                    mainQUEUE_RECEIVE_TASK_PRIORITY, NULL);
#endif
    }

    // uart_puts("任务创建完成，开始调度器...\n");
    vSendString("任务创建完成，开始调度器...\n");

    vTaskStartScheduler();

    return 0;
}