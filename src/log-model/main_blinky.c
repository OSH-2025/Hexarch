/*
 * Conditional compilation log module usage instructions:
 * 
 * 1. Enable log module: Set configUSE_LOG_MODULE = 1 in FreeRTOSConfig.h
 *    or compile with make ENABLE_LOG=1
 *    - Will create scheduler_test_task, using full log functionality
 *    - Log functions work normally, providing detailed debug information
 * 
 * 2. Disable log module: Set configUSE_LOG_MODULE = 0 in FreeRTOSConfig.h
 *    or compile with make ENABLE_LOG=0
 *    - Will create simple_demo_task, using simple serial output
 *    - Log function calls are optimized away, saving resources
 * 
 * Compilation command examples:
 * make clean && make ENABLE_LOG=1  # Enabled log version
 * make clean && make ENABLE_LOG=0  # Disabled log version
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

#include <stdio.h>

#include "riscv-virt.h"
#include "ns16550.h"
#include "log.h"

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

/* Global counter for logging */
static int counter = 0;

/*-----------------------------------------------------------*/

/* Simple integer to string function */
static void int_to_string(int num, char *buffer) {
    char tmp[12];
    int i = 0;
    
    /* Special case: 0 */
    if (num == 0) {
        buffer[0] = '0';
        buffer[1] = '\0';
        return;
    }
    
    /* Convert number to string (reverse order) */
    while (num > 0) {
        tmp[i++] = '0' + (num % 10);
        num /= 10;
    }
    
    /* Reverse string */
    int j = 0;
    while (i > 0) {
        buffer[j++] = tmp[--i];
    }
    buffer[j] = '\0';
}

/* Scheduler test task - does not depend on hardware GPIO */
void scheduler_test_task(void *pParam) {
    int count = 0;
    int task_id = (int)pParam;
    char task_name[20] = "SCHED-";
    char id_str[5];
    
    /* Construct task name */
    int_to_string(task_id, id_str);
    int i = 6; // Length of "SCHED-"
    int j = 0;
    while (id_str[j]) {
        task_name[i++] = id_str[j++];
    }
    task_name[i] = '\0';
    
    /* Use log API to record task startup */
    log_info_str("SCHED", "Starting scheduler test task", task_name);
    
    while(count < 20) {
        count++;
        
        /* Use log API to record count */
        log_info_int(task_name, "Loop count", count);
        
        /* Every 5 loops, voluntarily give up CPU to test scheduling */
        if (count % 5 == 0) {
            log_info(task_name, "Voluntarily giving up CPU");
            taskYIELD();
        }
        
        /* Add delay to avoid task executing too frequently */
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    
    /* Record completion and delete self */
    log_info_str("SCHED", "Task completed, about to delete", task_name);
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
    
    /* Construct task name */
    int_to_string(task_id, id_str);
    int i = 5; // Length of "Task-"
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
        /* Use log API to record count */
        log_info_int(task_name, "Loop count", counter);

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
    
    /* Construct task name */
    int_to_string(task_id, id_str);
    int i = 5; // Length of "Task-"
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
            /* Use log API to record count */
            log_info_int(task_name, "Loop count", counter);

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
    log_info_str("SCHED", "Starting scheduler test task", "task_name");

    /* Initialize log system */
    LogConfig_t log_config = {
        .level = LOG_LEVEL_INFO,
        .show_timestamp = 1  
    };
    
    if (log_init(&log_config)) {
        vSendString("Log system initialization successful\n");
    } else {
        vSendString("Log system initialization failed\n");
    }
#else
    vSendString("Log module disabled\n");
#endif
    
    /* Create the queue. */
    xQueue = xQueueCreate( mainQUEUE_LENGTH, sizeof( uint32_t ) );

    if( xQueue != NULL )
    {
#if (configUSE_LOG_MODULE == 1)
        /* Create producer and consumer tasks using logs */
        xTaskCreate( log_send_task, "Rx", configMINIMAL_STACK_SIZE * 2U, (void *)1,
                     mainQUEUE_SEND_TASK_PRIORITY, NULL );
        xTaskCreate( log_recv_task, "Tx", configMINIMAL_STACK_SIZE * 2U, (void *)2,
                     mainQUEUE_RECEIVE_TASK_PRIORITY, NULL );
        /* Create scheduler test tasks using logs */
        xTaskCreate( scheduler_test_task, "SCHED_TEST1", configMINIMAL_STACK_SIZE * 2U, (void *)1, 
                    2, NULL);
        xTaskCreate( scheduler_test_task, "SCHED_TEST2", configMINIMAL_STACK_SIZE * 2U, (void *)2, 
                    2, NULL);
#else
        /* Create producer and consumer tasks without using logs */
        xTaskCreate( prvQueueSendTask, "Rx", configMINIMAL_STACK_SIZE * 2U, (void *)1, 
                    mainQUEUE_SEND_TASK_PRIORITY, NULL);
        xTaskCreate( prvQueueReceiveTask, "Tx", configMINIMAL_STACK_SIZE * 2U, (void *)2, 
                    mainQUEUE_RECEIVE_TASK_PRIORITY, NULL);
#endif
    }
    vSendString("Task creation completed, starting scheduler...\n");

    vTaskStartScheduler();

    return 0;
}