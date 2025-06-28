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
#include <semphr.h>
#include <string.h>

/* 定义是否启用 FatFS 文件系统测试 */
#define USE_FATFS 0

/* Run a simple demo just prints 'Blink' */
#define DEMO_BLINKY               1
#define mainVECTOR_MODE_DIRECT    1

#if USE_FATFS
/* FatFS 相关头文件 */
#include "../../FatFS/ff.h"
#include "../../FatFS/diskio.h"
#endif

extern void freertos_risc_v_trap_handler( void );
extern void freertos_vector_table( void );

void vApplicationMallocFailedHook( void );
void vApplicationIdleHook( void );
void vApplicationStackOverflowHook( TaskHandle_t pxTask,
                                    char * pcTaskName );
void vApplicationTickHook( void );

/* 声明用于发送字符串的函数，在 main_blinky.c 中定义 */
extern void vSendString(const char *str);

/* 声明 main_blinky 中的任务创建函数，不要调用其中的 vTaskStartScheduler */
extern QueueHandle_t xCreateBlinkyTasks(void);

#if USE_FATFS
/* FatFS 测试任务的函数原型 */
static void prvFatFSTestTask( void *pvParameters );
#endif

/*-----------------------------------------------------------*/

int main( void )
{
    /* trap handler initialization */
    #if ( mainVECTOR_MODE_DIRECT == 1 )
    {
        __asm__ volatile ( "csrw mtvec, %0" : : "r" ( freertos_risc_v_trap_handler ) );
    }
    #else
    {
        __asm__ volatile ( "csrw mtvec, %0" : : "r" ( ( uintptr_t ) freertos_vector_table | 0x1 ) );
    }
    #endif

    vSendString( "Hello FreeRTOS!" );
    
    /* 创建 Blinky 任务 */
    QueueHandle_t xQueue = xCreateBlinkyTasks();
    
    #if USE_FATFS
    /* 创建 FatFS 测试任务 */
    vSendString("【FatFS】正在创建FatFS测试任务...");
    if (xTaskCreate(prvFatFSTestTask, "FatFS", configMINIMAL_STACK_SIZE * 8, NULL, 
                tskIDLE_PRIORITY + 3, NULL) != pdPASS) {
        vSendString("【FatFS】FatFS测试任务创建失败!");
    }
    #endif
    
    /* 启动调度器 */
    vTaskStartScheduler();

    /* 如果一切正常，执行不会到达这里 */
    for( ; ; );
    
    return 0;
}

#if USE_FATFS
/*-----------------------------------------------------------*/
/* FatFS 测试任务 */
static void prvFatFSTestTask( void *pvParameters )
{
    FATFS fs;           /* 文件系统对象 */
    FIL fil;            /* 文件对象 */
    DIR dir;            /* 目录对象 */
    FRESULT fr;         /* FatFS API返回值 */
    UINT bw, br;        /* 读写字节数 */
    char buffer[100];   /* 文件读写缓冲区 */
    FILINFO fno;        /* 文件信息对象 */
    BYTE work[FF_MAX_SS]; /* 工作区，用于格式化 */
    MKFS_PARM fmt_opt;  /* 格式化选项 */
    char msgBuf[100];   /* 消息缓冲区 */
    
    /* 延迟启动，确保其他任务已初始化 */
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    vSendString("【FatFS】测试开始...");
    
    /* 确保首先清除挂载 */
    f_mount(NULL, "", 0);
    
    /* 初始化磁盘 */
    if (disk_initialize(0) & STA_NOINIT) {
        vSendString("【FatFS】磁盘初始化失败!");
        vTaskDelete(NULL);
        return;
    }
    
    /* 格式化磁盘 */
    vSendString("【FatFS】正在格式化磁盘...");
    memset(&fmt_opt, 0, sizeof(fmt_opt));
    fmt_opt.fmt = FM_FAT | FM_SFD;  /* 使用 FAT 格式和单一 FAT 表 */
    
    fr = f_mkfs("", &fmt_opt, work, sizeof(work));
    if (fr != FR_OK) {
        sprintf(msgBuf, "【FatFS】格式化失败! (%d)", fr);
        vSendString(msgBuf);
        vTaskDelete(NULL);
        return;
    } else {
        vSendString("【FatFS】格式化成功");
    }
    
    /* 确保格式化完成后重新初始化磁盘 */
    disk_initialize(0);
    
    /* 挂载文件系统 */
    vSendString("【FatFS】正在挂载文件系统...");
    memset(&fs, 0, sizeof(FATFS));  /* 确保文件系统对象被正确初始化 */
    fr = f_mount(&fs, "", 1);  /* 1: 立即挂载 */
    
    if (fr != FR_OK) {
        sprintf(msgBuf, "【FatFS】挂载失败! (%d)", fr);
        vSendString(msgBuf);
        vTaskDelete(NULL);
        return;
    }
    
    vSendString("【FatFS】文件系统挂载成功!");
    
    /* 显示 FAT 类型 */
    sprintf(msgBuf, "【FatFS】FAT类型: %s", 
            (fs.fs_type == FS_FAT12) ? "FAT12" : 
            (fs.fs_type == FS_FAT16) ? "FAT16" : 
            (fs.fs_type == FS_FAT32) ? "FAT32" : 
            (fs.fs_type == FS_EXFAT) ? "exFAT" : "未知");
    vSendString(msgBuf);
    
    /* 测试1: 写入文件 */
    vSendString("【FatFS】测试1: 创建并写入文件 'TEST.TXT'...");
    fr = f_open(&fil, "TEST.TXT", FA_CREATE_ALWAYS | FA_WRITE);
    if (fr == FR_OK) {
        const char *message = "这是一个FatFS文件系统测试文件.";
        fr = f_write(&fil, message, strlen(message), &bw);
        if (fr == FR_OK) {
            sprintf(msgBuf, "【FatFS】写入成功: %u 字节", bw);
            vSendString(msgBuf);
        } else {
            sprintf(msgBuf, "【FatFS】写入失败! (%d)", fr);
            vSendString(msgBuf);
        }
        f_close(&fil);
    } else {
        sprintf(msgBuf, "【FatFS】文件创建失败! (%d)", fr);
        vSendString(msgBuf);
    }
    
    /* 测试2: 读取文件 */
    vSendString("【FatFS】测试2: 读取文件 'TEST.TXT'...");
    fr = f_open(&fil, "TEST.TXT", FA_READ);
    if (fr == FR_OK) {
        memset(buffer, 0, sizeof(buffer));
        fr = f_read(&fil, buffer, sizeof(buffer) - 1, &br);
        if (fr == FR_OK) {
            sprintf(msgBuf, "【FatFS】读取成功: %u 字节", br);
            vSendString(msgBuf);
            sprintf(msgBuf, "【FatFS】文件内容: %s", buffer);
            vSendString(msgBuf);
        } else {
            sprintf(msgBuf, "【FatFS】读取失败! (%d)", fr);
            vSendString(msgBuf);
        }
        f_close(&fil);
    } else {
        sprintf(msgBuf, "【FatFS】文件打开失败! (%d)", fr);
        vSendString(msgBuf);
    }
    
    /* 测试3: 创建目录 */
    vSendString("【FatFS】测试3: 创建目录 'TESTDIR'...");
    fr = f_mkdir("TESTDIR");
    if (fr == FR_OK || fr == FR_EXIST) {
        vSendString("【FatFS】目录创建成功或已存在");
    } else {
        sprintf(msgBuf, "【FatFS】目录创建失败! (%d)", fr);
        vSendString(msgBuf);
    }
    
    /* 测试4: 列出根目录内容 */
    vSendString("【FatFS】测试4: 列出根目录内容...");
    fr = f_opendir(&dir, "");
    if (fr == FR_OK) {
        while (1) {
            fr = f_readdir(&dir, &fno);
            if (fr != FR_OK || fno.fname[0] == 0) break;
            if (fno.fattrib & AM_DIR) {
                sprintf(msgBuf, "【FatFS】目录: %s", fno.fname);
                vSendString(msgBuf);
            } else {
                sprintf(msgBuf, "【FatFS】文件: %s, 大小: %lu 字节", fno.fname, fno.fsize);
                vSendString(msgBuf);
            }
        }
        f_closedir(&dir);
    } else {
        sprintf(msgBuf, "【FatFS】打开目录失败! (%d)", fr);
        vSendString(msgBuf);
    }
    
    /* 卸载文件系统 */
    vSendString("【FatFS】测试完成，卸载文件系统");
    f_mount(0, "", 0);
    
    vSendString("【FatFS】FatFS 集成测试成功完成!");
    
    /* 测试完成后，删除任务 */
    vTaskDelete(NULL);
}
#endif

/*-----------------------------------------------------------*/

void vApplicationMallocFailedHook( void )
{
    /* vApplicationMallocFailedHook() will only be called if
     * configUSE_MALLOC_FAILED_HOOK is set to 1 in FreeRTOSConfig.h.  It is a hook
     * function that will get called if a call to pvPortMalloc() fails.
     * pvPortMalloc() is called internally by the kernel whenever a task, queue,
     * timer or semaphore is created.  It is also called by various parts of the
     * demo application.  If heap_1.c or heap_2.c are used, then the size of the
     * heap available to pvPortMalloc() is defined by configTOTAL_HEAP_SIZE in
     * FreeRTOSConfig.h, and the xPortGetFreeHeapSize() API function can be used
     * to query the size of free heap space that remains (although it does not
     * provide information on how the remaining heap might be fragmented). */
    taskDISABLE_INTERRUPTS();

    for( ; ; )
    {
    }
}
/*-----------------------------------------------------------*/

void vApplicationIdleHook( void )
{
    /* vApplicationIdleHook() will only be called if configUSE_IDLE_HOOK is set
     * to 1 in FreeRTOSConfig.h.  It will be called on each iteration of the idle
     * task.  It is essential that code added to this hook function never attempts
     * to block in any way (for example, call xQueueReceive() with a block time
     * specified, or call vTaskDelay()).  If the application makes use of the
     * vTaskDelete() API function (as this demo application does) then it is also
     * important that vApplicationIdleHook() is permitted to return to its calling
     * function, because it is the responsibility of the idle task to clean up
     * memory allocated by the kernel to any task that has since been deleted. */
}
/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook( TaskHandle_t pxTask,
                                    char * pcTaskName )
{
    ( void ) pcTaskName;
    ( void ) pxTask;

    /* Run time stack overflow checking is performed if
     * configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
     * function is called if a stack overflow is detected. */
    taskDISABLE_INTERRUPTS();

    for( ; ; )
    {
    }
}
/*-----------------------------------------------------------*/

void vApplicationTickHook( void )
{
}
/*-----------------------------------------------------------*/

void vAssertCalled( void )
{
    volatile uint32_t ulSetTo1ToExitFunction = 0;

    taskDISABLE_INTERRUPTS();

    while( ulSetTo1ToExitFunction != 1 )
    {
        __asm volatile ( "NOP" );
    }
}

