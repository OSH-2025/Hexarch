/*-----------------------------------------------------------------------*/
/* Low level disk I/O module SKELETON for FatFs     (C)ChaN, 2019        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "ff.h"			/* Obtains integer types */
#include "diskio.h"		/* Declarations of disk functions */
#include "FreeRTOS.h"
#include <string.h>

/* 添加调试输出 */
extern void uart_puts(const char* s);
extern void uart_putc(char c);
extern void uart_print_num(FRESULT num);

/* Definitions of physical drive number for each drive */
#define DEV_RAM		0	/* Example: Map Ramdisk to physical drive 0 */
//#define DEV_MMC		1	/* Example: Map MMC/SD card to physical drive 1 */
//#define DEV_USB		2	/* Example: Map USB MSD to physical drive 2 */

#define SECTOR_SIZE 512
#define SECTOR_COUNT 2048

#define RAMDISK_SIZE (SECTOR_SIZE * SECTOR_COUNT)

/* 静态分配 RAM 磁盘，避免动态内存分配问题 */
static BYTE ramdisk[RAMDISK_SIZE];
static DSTATUS Stat = STA_NOINIT; /* 磁盘状态 */

/* 添加初始化标志，确保只初始化一次 */
static int disk_initialized = 0;

/* 添加调试标志，减少输出频率 */
static int debug_output_enabled = 0;

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status(BYTE pdrv) {
    if (pdrv != DEV_RAM) return STA_NOINIT;  /* 仅支持 RAM 磁盘 */
    
    /* 减少调试输出，避免死循环 */
    return Stat;
}

/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize(BYTE pdrv) {
    if (pdrv != DEV_RAM) {
        if (debug_output_enabled) {
            uart_puts("disk_initialize: Invalid drive\n");
        }
        return STA_NOINIT;  /* 仅支持 RAM 磁盘 */
    }
    
    /* 只在第一次初始化时清零RAM磁盘 */
    if (!disk_initialized) {
        if (debug_output_enabled) {
            uart_puts("disk_initialize: First initialization, clearing RAM disk\n");
        }
        memset(ramdisk, 0, RAMDISK_SIZE);
        disk_initialized = 1;
    }
    
    /* 清除所有状态标志，确保不会有写保护或其他状态 */
    Stat = 0;
    
    if (debug_output_enabled) {
        uart_puts("disk_initialize: Success\n");
    }
    return Stat;  /* 总是返回成功 */
}

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read(BYTE pdrv, BYTE* buff, LBA_t sector, UINT count) {
    if (pdrv != DEV_RAM) {
        return RES_PARERR;  /* 检查参数 */
    }
    
    if (Stat & STA_NOINIT) {
        return RES_NOTRDY;  /* 驱动器未初始化 */
    }
    
    if (sector >= SECTOR_COUNT || sector + count > SECTOR_COUNT) {
        return RES_PARERR;  /* 超出范围 */
    }
    
    /* 从 RAM 磁盘拷贝数据到缓冲区 */
    memcpy(buff, ramdisk + sector * SECTOR_SIZE, count * SECTOR_SIZE);
    
    return RES_OK;
}

/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

DRESULT disk_write(BYTE pdrv, const BYTE* buff, LBA_t sector, UINT count) {
    if (pdrv != DEV_RAM) {
        return RES_PARERR;  /* 检查参数 */
    }
    
    if (Stat & STA_NOINIT) {
        return RES_NOTRDY;  /* 驱动器未初始化 */
    }
    
    if (sector >= SECTOR_COUNT || sector + count > SECTOR_COUNT) {
        return RES_PARERR;  /* 超出范围 */
    }
    
    /* 从缓冲区拷贝数据到 RAM 磁盘 */
    memcpy(ramdisk + sector * SECTOR_SIZE, buff, count * SECTOR_SIZE);
    
    return RES_OK;
}

/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void* buff) {
    DRESULT res = RES_PARERR;
    
    if (pdrv != DEV_RAM) {
        return RES_PARERR;  /* 检查参数 */
    }
    
    if (Stat & STA_NOINIT) {
        return RES_NOTRDY;  /* 驱动器未初始化 */
    }
    
    switch (cmd) {
        case CTRL_SYNC:        /* 确保写入完成 */
            res = RES_OK;      /* RAM 磁盘不需要同步 */
            break;
            
        case GET_SECTOR_COUNT: /* 获取媒体容量（以扇区为单位） */
            *(DWORD*)buff = SECTOR_COUNT;
            res = RES_OK;
            break;
            
        case GET_SECTOR_SIZE:  /* 获取扇区大小 */
            *(WORD*)buff = SECTOR_SIZE;
            res = RES_OK;
            break;
            
        case GET_BLOCK_SIZE:   /* 获取擦除块大小 */
            *(DWORD*)buff = 1; /* 不可擦除，返回1 */
            res = RES_OK;
            break;
            
        default:
            res = RES_OK;  /* 对于未知命令，返回OK而不是错误 */
            break;
    }
    
    return res;
}

