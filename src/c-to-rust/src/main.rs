#![no_std]
#![no_main]
#![allow(non_snake_case)]
#![feature(core_intrinsics)]
#![feature(alloc_error_handler)]
#![feature(const_trait_impl)]
extern crate libc;
use libc::*;


extern crate alloc;
use alloc::format;
use alloc::sync::{Arc,Weak};
use core::clone::Clone;
use core::default::Default;
use alloc::string;
use core::option::Option;

extern crate embedded_alloc;

use embedded_alloc::LlffHeap as Heap;

#[global_allocator]
static HEAP: Heap = Heap::empty();

use core::panic::PanicInfo;

// panic 处理函数 - 必须实现
#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    // 这里可以实现自定义的 panic 行为
    // 例如打印错误信息、重置系统等
    loop {} // 简单地进入无限循环
}

fn main() {
    // Initialize the allocator BEFORE you use it
    // now the allocator is ready types like Box, Vec can be used.

}