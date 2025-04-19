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
#include <stdio.h>

/* Definitions of physical drive number for each drive */
//#define DEV_RAM		0	/* Example: Map Ramdisk to physical drive 0 */
//#define DEV_MMC		1	/* Example: Map MMC/SD card to physical drive 1 */
//#define DEV_USB		2	/* Example: Map USB MSD to physical drive 2 */

#define SECTOR_SIZE 512
#define SECTOR_COUNT 32768
static FILE *fp = NULL;

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status(BYTE pdrv) {
    return (fp == NULL) ? STA_NOINIT : 0;
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize(BYTE pdrv) {
    fp = fopen("virtual_disk.img", "r+b");
    return (fp == NULL) ? STA_NOINIT : 0;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read(BYTE pdrv, BYTE* buff, LBA_t sector, UINT count) {
    if (!fp) return RES_NOTRDY;
    fseek(fp, sector * SECTOR_SIZE, SEEK_SET);
    return (fread(buff, SECTOR_SIZE, count, fp) == count) ? RES_OK : RES_ERROR;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

DRESULT disk_write(BYTE pdrv, const BYTE* buff, LBA_t sector, UINT count) {
    if (!fp) return RES_NOTRDY;
    fseek(fp, sector * SECTOR_SIZE, SEEK_SET);
    return (fwrite(buff, SECTOR_SIZE, count, fp) == count) ? RES_OK : RES_ERROR;
}


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void* buff) {
    if (!fp) return RES_NOTRDY;
    switch (cmd) {
        case GET_SECTOR_SIZE: *(WORD*)buff = SECTOR_SIZE; return RES_OK;
        case GET_SECTOR_COUNT: *(DWORD*)buff = SECTOR_COUNT; return RES_OK;
        default: return RES_PARERR;
    }
}

