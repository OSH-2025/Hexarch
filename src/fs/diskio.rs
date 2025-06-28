// filepath: /home/ustc/Desktop/RFREERTOS-master origin/RFREERTOS-master origin/RFREERTOS-master/src/kernel/fs/diskio.rs
//! Rust implementation of the diskio.h interface for FatFs

use crate::fs::ramdisk;
use crate::print;
use core::ffi::c_uint;


// 这些是 FatFs diskio.h 中定义的常量
pub const STA_NOINIT: u8 = 0x01;
pub const RES_OK: u8 = 0x00;
pub const RES_ERROR: u8 = 0x01;

// 添加 ioctl 命令常量
pub const CTRL_SYNC: u8 = 0;
pub const GET_SECTOR_COUNT: u8 = 1;
pub const GET_SECTOR_SIZE: u8 = 2;
pub const GET_BLOCK_SIZE: u8 = 3;

#[no_mangle]
pub extern "C" fn disk_initialize(_pdrv: u8) -> u8 {
    print("[FATFS] disk_initialize called\n");
    RES_OK
}

#[no_mangle]
pub extern "C" fn disk_status(_pdrv: u8) -> u8 {
    0//RAM Disk总是准备就绪
}

#[no_mangle]
pub extern "C" fn disk_read(
    _pdrv: u8,
    buff: *mut u8,
    sector: u32,
    count: c_uint,
) -> u8 {
    let slice = unsafe {
        core::slice::from_raw_parts_mut(buff, (count as usize) * 512)
    };
    if ramdisk::read_sectors(sector, count as usize, slice).is_ok() {
        RES_OK
    }else {
        RES_ERROR
    }
}

#[no_mangle]
pub extern "C" fn disk_write(
    _pdrv: u8,
    buff: *const u8,
    sector: u32,
    count: c_uint,
) -> u8 {
    let slice = unsafe {
        core::slice::from_raw_parts(buff, (count as usize) * 512)
    };
    if ramdisk::write_sectors(sector, count as usize, slice).is_ok() {
        RES_OK
    } else {
        RES_ERROR
    }
}

// 添加缺失的 disk_ioctl 函数
#[no_mangle]
pub extern "C" fn disk_ioctl(
    _pdrv: u8,
    cmd: u8,
    buff: *mut core::ffi::c_void,
) -> u8 {
    match cmd {
        CTRL_SYNC => {
            // 对于 RAM disk，同步操作不需要做任何事情
            RES_OK
        },
        GET_SECTOR_COUNT => {
            // 返回扇区总数
            if !buff.is_null() {
                unsafe {
                    *(buff as *mut u32) = 256; // 我们的 RAM disk 有 256 个扇区
                }
            }
            RES_OK
        },
        GET_SECTOR_SIZE => {
            // 返回扇区大小
            if !buff.is_null() {
                unsafe {
                    *(buff as *mut u32) = 512; // 每个扇区 512 字节
                }
            }
            RES_OK
        },
        GET_BLOCK_SIZE => {
            // 返回擦除块大小
            if !buff.is_null() {
                unsafe {
                    *(buff as *mut u32) = 1; // 对于 RAM disk，块大小为 1 个扇区
                }
            }
            RES_OK
        },
        _ => {
            RES_ERROR // 不支持的命令
        }
    }
}

#[no_mangle]
pub extern "C" fn get_fattime() -> u32 {
    // 返回一个固定的时间戳，或者实现RTC驱动来获取真实时间
    ((2025 - 1980) << 25) | (6 << 21) | (28 << 16)
}