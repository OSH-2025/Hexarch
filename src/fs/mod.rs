//这个文件是fs模块的入口文件，它有两个核心作用
// 1. 声明diskio和sync子模块
pub mod diskio;
pub mod sync;
pub mod ramdisk;

// 2. FatFs FFI (Foreign Function Interface) 绑定
//    这里我们为 ff.h 中的函数和结构体创建 Rust 定义
use core::ffi::{c_char, c_int, c_uint, c_void};

//常量定义，来自 FatFs ff.h
pub const FA_READ: u8 = 0x01;
pub const FA_WRITE: u8 = 0x02;
pub const FA_OPEN_EXISTING: u8 = 0x00;
pub const FA_CREATE_NEW: u8 = 0x04;
pub const FA_CREATE_ALWAYS: u8 = 0x08;
pub const FA_OPEN_ALWAYS: u8 = 0x10;

pub const FR_OK: u8 = 0;

// --- 结构体定义 (来自 ff.h) ---
// 使用 #[repr(C)] 确保内存布局与 C 语言兼容
#[repr(C)]
pub struct FATFS {
    // 我们不需要关心内部细节，只需保证大小和对齐正确。
    // 用一个足够大的字节数组作为占位符即可。
    _private: [u8; 512], // 这是一个示例大小，具体需要看 ff.h
}

#[repr(C)]
pub struct FIL {
    _private: [u8; 512], // 同上
}

// --- 新增: f_mkfs 所需的参数结构体 ---
#[repr(C)]
pub struct MKFS_PARM {
    pub fmt: u8,
    pub n_fat: u8,
    pub align: u32,
    pub n_root: u32,
    pub au_size: u32,
}


// --- 函数声明 (来自 ff.h) ---
// 告诉 Rust 这些函数是在外部库（我们编译的 libfatfs.a）中定义的
#[link(name = "fatfs")]
extern "C" {
    pub fn f_mount(fs: *mut FATFS, path: *const c_char, opt: u8) -> u8;
    pub fn f_open(fp: *mut FIL, path: *const c_char, mode: u8) -> u8;
    pub fn f_close(fp: *mut FIL) -> u8;
    pub fn f_write(
        fp: *mut FIL,
        buff: *const c_void,
        btw: c_uint,
        bw: *mut c_uint,
    ) -> u8;
    pub fn f_read(
        fp: *mut FIL,
        buff: *mut c_void,
        btr: c_uint,
        br: *mut c_uint,
    ) -> u8;
    // --- 新增: f_mkfs 的 FFI 声明 ---
    pub fn f_mkfs(path: *const c_char, opt: *const MKFS_PARM, work: *mut c_void, len: c_uint) -> u8;
}