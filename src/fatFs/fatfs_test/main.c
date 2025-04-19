#include "ff.h"
#include <stdio.h>
#include <string.h>

FATFS fs;    // 文件系统对象
FIL file;    // 文件对象

// 声明 get_fattime 函数
DWORD get_fattime(void);

int main(void) {
    FRESULT res;
    UINT bw, br;
    char buffer[100];
    
    // 打印开始信息
    printf("FatFS Test Started.\n");
    
    // 1. 挂载文件系统
    printf("Mounting the file system...\n");
    res = f_mount(&fs, "", 1); // 挂载文件系统
    if (res != FR_OK) {
        printf("Error mounting file system: %d\n", res);
        return 1;
    } else {
        printf("File system mounted successfully!\n");
    }

    // 2. 创建并写入文件
    printf("Creating and writing to the file...\n");
    res = f_open(&file, "test.txt", FA_CREATE_ALWAYS | FA_WRITE);  // 创建文件并写入
    if (res != FR_OK) {
        printf("Error opening file: %d\n", res);
        return 1;
    }
    char data[] = "Hello, FatFS!";
    res = f_write(&file, data, strlen(data), &bw);  // 写入数据
    if (res != FR_OK) {
        printf("Error writing to file: %d\n", res);
        return 1;
    } else {
        printf("Data written to file successfully! Bytes written: %u\n", bw);
    }
    f_close(&file);

    // 3. 读取文件内容
    printf("Reading the file...\n");
    res = f_open(&file, "test.txt", FA_READ);  // 打开文件进行读取
    if (res != FR_OK) {
        printf("Error opening file for reading: %d\n", res);
        return 1;
    }
    res = f_read(&file, buffer, sizeof(buffer), &br);  // 读取文件数据
    if (res != FR_OK) {
        printf("Error reading from file: %d\n", res);
        return 1;
    } else {
        buffer[br] = '\0';  // 确保字符串结尾
        printf("Data read from file: %s\n", buffer);
    }
    f_close(&file);

    // 4. 获取磁盘剩余空间
    DWORD fre_clust;
    FATFS *fs;
    res = f_getfree("0:", &fre_clust, &fs);  // 获取剩余空间
    if (res != FR_OK) {
        printf("Error getting free space: %d\n", res);
        return 1;
    } else {
        printf("Free clusters: %u\n", fre_clust);  // 使用 %u 以匹配 DWORD 类型
    }

    // 结束信息
    printf("FatFS Test Completed Successfully.\n");

    return 0;
}

