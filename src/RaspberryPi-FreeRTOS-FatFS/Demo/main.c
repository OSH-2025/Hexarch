/*
    FreeRTOS V7.2.0 - Copyright (C) 2012 Real Time Engineers Ltd.
	

    ***************************************************************************
     *                                                                       *
     *    FreeRTOS tutorial books are available in pdf and paperback.        *
     *    Complete, revised, and edited pdf reference manuals are also       *
     *    available.                                                         *
     *                                                                       *
     *    Purchasing FreeRTOS documentation will not only help you, by       *
     *    ensuring you get running as quickly as possible and with an        *
     *    in-depth knowledge of how to use FreeRTOS, it will also help       *
     *    the FreeRTOS project to continue with its mission of providing     *
     *    professional grade, cross platform, de facto standard solutions    *
     *    for microcontrollers - completely free of charge!                  *
     *                                                                       *
     *    >>> See http://www.FreeRTOS.org/Documentation for details. <<<     *
     *                                                                       *
     *    Thank you for using FreeRTOS, and thank you for your support!      *
     *                                                                       *
    ***************************************************************************


    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation AND MODIFIED BY the FreeRTOS exception.
    >>>NOTE<<< The modification to the GPL is included to allow you to
    distribute a combined work that includes FreeRTOS without being obliged to
    provide the source code for proprietary components outside of the FreeRTOS
    kernel.  FreeRTOS is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
    more details. You should have received a copy of the GNU General Public
    License and the FreeRTOS license exception along with FreeRTOS; if not it
    can be viewed here: http://www.freertos.org/a00114.html and also obtained
    by writing to Richard Barry, contact details for whom are available on the
    FreeRTOS WEB site.

    1 tab == 4 spaces!
    
    ***************************************************************************
     *                                                                       *
     *    Having a problem?  Start by reading the FAQ "My application does   *
     *    not run, what could be wrong?                                      *
     *                                                                       *
     *    http://www.FreeRTOS.org/FAQHelp.html                               *
     *                                                                       *
    ***************************************************************************

    
    http://www.FreeRTOS.org - Documentation, training, latest information, 
    license and contact details.
    
    http://www.FreeRTOS.org/plus - A selection of FreeRTOS ecosystem products,
    including FreeRTOS+Trace - an indispensable productivity tool.

    Real Time Engineers ltd license FreeRTOS to High Integrity Systems, who sell 
    the code with commercial support, indemnification, and middleware, under 
    the OpenRTOS brand: http://www.OpenRTOS.com.  High Integrity Systems also
    provide a safety engineered and independently SIL3 certified version under 
    the SafeRTOS brand: http://www.SafeRTOS.com.
*/


#include <FreeRTOS.h>
#include <task.h>
#include <string.h>

#include "Drivers/irq.h"
#include "Drivers/gpio.h"
#include "ff.h"           /* FatFS头文件 */

#define UART0_BASE  0x101f1000
#define UART0_DR    (*(volatile unsigned int*)(UART0_BASE + 0x00))
#define UART0_FR    (*(volatile unsigned int*)(UART0_BASE + 0x18))

void uart_putc(char c) {
    while (UART0_FR & (1 << 5)) ; // 等待发送 FIFO 非满
    UART0_DR = c;
}

void uart_puts(const char* s) {
    while (*s) {
        uart_putc(*s++);
    }
}

/* 简单的整数转字符串函数，避免使用 sprintf */
void uart_print_num(FRESULT num) {
    char buf[4];
    int i = 0;
    
    /* 处理0的特殊情况 */
    if (num == 0) {
        uart_putc('0');
        return;
    }
    
    /* 将数字转换为字符串（倒序） */
    while (num > 0) {
        buf[i++] = '0' + (num % 10);
        num /= 10;
    }
    
    /* 输出（反向输出以得到正确顺序） */
    while (i > 0) {
        uart_putc(buf[--i]);
    }
}

void print_fatfs_error(FRESULT res) {
    uart_puts("FatFS Error: ");
    switch (res) {
        case FR_OK: uart_puts("(0) Succeeded"); break;
        case FR_DISK_ERR: uart_puts("(1) A hard error occurred in the low level disk I/O layer"); break;
        case FR_INT_ERR: uart_puts("(2) Assertion failed"); break;
        case FR_NOT_READY: uart_puts("(3) The physical drive cannot work"); break;
        case FR_NO_FILE: uart_puts("(4) Could not find the file"); break;
        case FR_NO_PATH: uart_puts("(5) Could not find the path"); break;
        case FR_INVALID_NAME: uart_puts("(6) The path name format is invalid"); break;
        case FR_DENIED: uart_puts("(7) Access denied due to prohibited access or directory full"); break;
        case FR_EXIST: uart_puts("(8) Access denied due to prohibited access"); break;
        case FR_INVALID_OBJECT: uart_puts("(9) The file/directory object is invalid"); break;
        case FR_WRITE_PROTECTED: uart_puts("(10) The physical drive is write protected"); break;
        case FR_INVALID_DRIVE: uart_puts("(11) The logical drive number is invalid"); break;
        case FR_NOT_ENABLED: uart_puts("(12) The volume has no work area"); break;
        case FR_NO_FILESYSTEM: uart_puts("(13) There is no valid FAT volume"); break;
        case FR_MKFS_ABORTED: uart_puts("(14) The f_mkfs() aborted due to any problem"); break;
        case FR_TIMEOUT: uart_puts("(15) Could not get a grant to access the volume within defined period"); break;
        case FR_LOCKED: uart_puts("(16) The operation is rejected according to the file sharing policy"); break;
        case FR_NOT_ENOUGH_CORE: uart_puts("(17) LFN working buffer could not be allocated"); break;
        case FR_TOO_MANY_OPEN_FILES: uart_puts("(18) Number of open files > FF_FS_LOCK"); break;
        case FR_INVALID_PARAMETER: uart_puts("(19) Given parameter is invalid"); break;
        default: uart_puts("Unknown error code"); break;
    }
    uart_puts("\n");
}

void uart_init() {
    // 对于 QEMU versatilepb 平台，一般 UART 默认已初始化
    // 你可以留空，或者设定波特率等（如 PL011 初始化）
}

void task1(void *pParam) {
    (void)pParam;  /* 防止未使用参数警告 */

    int i = 0;
    while(1) {
        i++;
        SetGpio(16, 1);
        vTaskDelay(200);
    }
}

void task2(void *pParam) {
    (void)pParam;  /* 防止未使用参数警告 */

    int i = 0;
    while(1) {
        i++;
        vTaskDelay(100);
        SetGpio(16, 0);
        vTaskDelay(100);
    }
}

void fatfs_task(void *pParam) {
    FATFS fs;           /* 文件系统对象 */
    FIL fil;            /* 文件对象 */
    DIR dir;            /* 目录对象 */
    FRESULT res;        /* FatFS API返回值 */
    UINT bw;            /* 写入字节数 */
    char buffer[100];   /* 文件读写缓冲区 */
    BYTE work[FF_MAX_SS]; /* 工作区 */
    FILINFO fno;        /* 文件信息对象 */
    
    (void)pParam;       /* 防止未使用参数警告 */
    
    uart_puts("FatFS Task Started\n");
    
    /* 初始化磁盘 */
    disk_initialize(0);
    
    /* 确保首先清除挂载 */
    f_mount(NULL, "", 0);
    
    /* 格式化磁盘 */
    uart_puts("Formatting disk...\n");
    MKFS_PARM fmt_opt;
    memset(&fmt_opt, 0, sizeof(fmt_opt));
    fmt_opt.fmt = FM_FAT | FM_SFD;  /* FAT格式 + 单FAT表 */
    
    res = f_mkfs("", &fmt_opt, work, sizeof(work));
    uart_puts("Format result: ");
    if (res != FR_OK) {
        print_fatfs_error(res);
        vTaskDelete(NULL);
        return;
    } else {
        uart_puts("Success\n");
    }
    
    /* 挂载文件系统 */
    uart_puts("Mounting filesystem...\n");
    res = f_mount(&fs, "", 1);  /* 1: 立即挂载 */
    
    if (res != FR_OK) {
        uart_puts("Mount failed: ");
        print_fatfs_error(res);
        vTaskDelete(NULL);
        return;
    }
    
    uart_puts("Filesystem mounted successfully\n");
    
    /* 尝试写一个简单的测试文件 */
    uart_puts("Creating test file...\n");
    res = f_open(&fil, "TEST.TXT", FA_CREATE_ALWAYS | FA_WRITE);
    
    if (res != FR_OK) {
        uart_puts("Failed to create file: ");
        print_fatfs_error(res);
        
        /* 检查根目录访问 */
        uart_puts("Checking root directory...\n");
        res = f_opendir(&dir, "/");
        if (res != FR_OK) {
            uart_puts("Failed to open root directory: ");
            print_fatfs_error(res);
        } else {
            uart_puts("Root directory opened successfully\n");
            f_closedir(&dir);
        }
    } else {
        const char *message = "Hello from FreeRTOS and FatFS integration!\n";
        res = f_write(&fil, message, strlen(message), &bw);
        
        if (res != FR_OK) {
            uart_puts("Failed to write to file: ");
            print_fatfs_error(res);
        } else {
            uart_puts("File written successfully, ");
            uart_print_num(bw);
            uart_puts(" bytes written\n");
        }
        
        f_sync(&fil);  /* 确保数据写入磁盘 */
        f_close(&fil);
        
        /* 尝试读取文件 */
        uart_puts("Opening file for reading...\n");
        res = f_open(&fil, "TEST.TXT", FA_READ);
        
        if (res != FR_OK) {
            uart_puts("Failed to open file for reading: ");
            print_fatfs_error(res);
        } else {
            memset(buffer, 0, sizeof(buffer));
            
            res = f_read(&fil, buffer, sizeof(buffer), &bw);
            
            if (res != FR_OK) {
                uart_puts("Failed to read file: ");
                print_fatfs_error(res);
            } else {
                uart_puts("File content: ");
                uart_puts(buffer);
            }
            
            f_close(&fil);
        }
    }
    
    /* 任务完成后循环等待 */
    while(1) {
        vTaskDelay(1000);
    }
}

/**
 *	This is the systems main entry, some call it a boot thread.
 *
 *	-- Absolutely nothing wrong with this being called main(), just it doesn't have
 *	-- the same prototype as you'd see in a linux program.
 **/
void main (void)
{
    uart_init();
    uart_puts("Hello from FreeRTOS with FatFS!\n");

    SetGpioFunction(16, 1);			// RDY led

    xTaskCreate(task1, (const signed char *)"LED_0", 128, NULL, 0, NULL);
    xTaskCreate(task2, (const signed char *)"LED_1", 128, NULL, 0, NULL);
    xTaskCreate(fatfs_task, (const signed char *)"FatFS", 512, NULL, 1, NULL);
    	
    vTaskStartScheduler();

    /*
     *	We should never get here, but just in case something goes wrong,
     *	we'll place the CPU into a safe loop.
     */
    while(1) {
        ;
    }
}
