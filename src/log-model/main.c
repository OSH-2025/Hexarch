/*
 * Simple log system test program
 * Used to test log system functionality under FreeRTOS
 */

#include <FreeRTOS.h>
#include <task.h>
#include "Drivers/gpio.h"
#include "log.h"

#define UART0_BASE  0x101f1000
#define UART0_DR    (*(volatile unsigned int*)(UART0_BASE + 0x00))
#define UART0_FR    (*(volatile unsigned int*)(UART0_BASE + 0x18))

/* Basic UART output function */
void uart_putc(char c) {
    while (UART0_FR & (1 << 5)) ; // Wait for transmit FIFO not full
    UART0_DR = c;
}

void uart_puts(const char* s) {
    while (*s) {
        uart_putc(*s++);
    }
}

/* Task to test various log levels and types */
void log_test_task(void *pParam) {
    int count = 0;
    
    uart_puts("Starting log test task\n");
    
    while(count < 30) {
        count++;
        
        /* Basic log test */
        log_error("TEST", "This is an error log");
        log_info("TEST", "This is an info log");
        log_debug("TEST", "This is a debug log");
        
        /* Numeric log test */
        log_error_int("TEST", "Error log - integer value", count);
        log_info_int("TEST", "Info log - integer value", count * 10);
        log_debug_int("TEST", "Debug log - integer value", count * 100);
        
        /* Hexadecimal value test */
        log_error_hex("TEST", "Error log - hexadecimal value", count);
        log_info_hex("TEST", "Info log - hexadecimal value", 0xABCD);
        log_debug_hex("TEST", "Debug log - hexadecimal value", 0x12345678);
        
        /* String value test */
        log_error_str("TEST", "Error log - string value", "Test error string");
        log_info_str("TEST", "Info log - string value", "Test info string");
        log_debug_str("TEST", "Debug log - string value", "Test debug string");
        
        /* Direct UART output for verification */
        uart_puts("\n==================\n");
        uart_puts("Completed log test cycle ");
        
        /* Simple number output */
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
        
        /* 5 second interval between each test */
        // vTaskDelay(500);
        
        /* Switch log level every 3 cycles */
        if (count % 3 == 0) {
            LogLevel_t current_level = log_get_level();
            
            /* Rotate log levels: ERROR -> INFO -> DEBUG -> ERROR ... */
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
    int counter = 0;
    int task_id = (int)pParam;
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
    
    /* Use log API to record task startup */
    log_info_str("SCHED", "Starting scheduler test task", task_name);
    
    while(counter < 50) {
        counter++;
        
        /* Use log API to record count */
        log_info_int(task_name, "Loop count", counter);

        /* Every 5 loops, voluntarily give up CPU to test scheduling */
        if (counter % 5 == 0) {
            log_info(task_name, "Voluntarily giving up CPU");
            taskYIELD();
        }
    }
    return ;
}

/**
 * System entry function
 */
void main(void) {
    /* Initialize UART (usually no additional configuration needed in QEMU) */
    uart_puts("\n\n=== Log System Test Program Started ===\n\n");
    
    /* Initialize LED GPIO */
    SetGpioFunction(16, 1);
    
    /* Initialize log system */
    LogConfig_t log_config = {
        .level = LOG_LEVEL_INFO,
        .show_timestamp = true
    };
    
    if (log_init(&log_config)) {
        uart_puts("Log system initialization successful\n");
    } else {
        uart_puts("Log system initialization failed\n");
    }
    
    /* Create test tasks */
    xTaskCreate(log_test_task, (const signed char *)"LOG_TEST", 512, NULL, 3, NULL); // High priority task
    
    /* Create multiple scheduler test tasks with different priorities and parameters */
    xTaskCreate(scheduler_test_task, (const signed char *)"SCHED_TEST1", 256, (void *)1, 2, NULL); // Low priority
    xTaskCreate(scheduler_test_task, (const signed char *)"SCHED_TEST2", 256, (void *)2, 2, NULL); // Medium priority
    
    uart_puts("Task creation completed, starting scheduler...\n");
    
    /* Start FreeRTOS scheduler */
    vTaskStartScheduler();
    
    /* If scheduler fails, enter safe loop */
    while(1) {
        ;
    }
}