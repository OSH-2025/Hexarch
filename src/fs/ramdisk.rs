//! A simple RAM-based block device for demonstration.

use crate::print;
use spin::Mutex;

// 定义磁盘大小：256个扇区 * 512字节/扇区 = 128 KB
const SECTOR_SIZE: usize = 512;
const SECTOR_COUNT: usize = 256;

// 这就是我们的 RAM Disk！一个静态的、线程安全的内存块。
static RAM_DISK: Mutex<[u8; SECTOR_COUNT * SECTOR_SIZE]> = Mutex::new([0; SECTOR_COUNT * SECTOR_SIZE]);

/// 读取多个扇区
pub fn read_sectors(sector_id: u32, count: usize, buf: &mut [u8]) -> Result<(), ()> {
    if sector_id as usize + count > SECTOR_COUNT {
        print("[RAMDISK] Error: Read out of bounds!");
        return Err(());
    }
    let start = sector_id as usize * SECTOR_SIZE;
    let end = start + count * SECTOR_SIZE;
    let disk = RAM_DISK.lock();
    buf[..count * SECTOR_SIZE].copy_from_slice(&disk[start..end]);
    Ok(())
}

/// 写入多个扇区
pub fn write_sectors(sector_id: u32, count: usize, buf: &[u8]) -> Result<(), ()> {
    if sector_id as usize + count > SECTOR_COUNT {
        print("[RAMDISK] Error: Write out of bounds!");
        return Err(());
    }
    let start = sector_id as usize * SECTOR_SIZE;
    let end = start + count * SECTOR_SIZE;
    let mut disk = RAM_DISK.lock();
    disk[start..end].copy_from_slice(&buf[..count * SECTOR_SIZE]);
    Ok(())
}