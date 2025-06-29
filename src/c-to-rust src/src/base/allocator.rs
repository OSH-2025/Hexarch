//! Global Allocatot Definition <br>
//! use buddy_system_allocator and wrap as RustAllocator
use crate::tasks::{vTaskEnterCritical, vTaskExitCritical};
use crate::base::FreeRTOSconfig::KERNEL_HEAP_SIZE;
use alloc::format;
use buddy_system_allocator::LockedHeap;
use core::alloc::{GlobalAlloc, Layout};

/// INITIAL Start should init_heap first
pub fn init_heap() {
    //静态可变变量,在 Rust 中，static mut 变量被认为是不安全的
    static mut HEAP: [u8; KERNEL_HEAP_SIZE] = [0; KERNEL_HEAP_SIZE];
    //内存池初始化
    //在系统启动时调用 init_heap() 初始化分配器
    unsafe {
        //使用 unsafe 块来明确告诉编译器：我知道这里可能有风险，但我保证代码是安全的
        DYNAMIC_ALLOCATOR //访问静态可变变量
            .Buddy_System_Allocator
            .lock()
            .init(HEAP.as_ptr() as usize, KERNEL_HEAP_SIZE); //原始指针操作 + 直接操作内存地址
    }
}
//在 no_std 环境中，Rust 不提供标准库的内存分配功能，因此需要手动实现全局内存分配器
#[global_allocator]
/// DYNAMIC_ALLOCATOR as global_allocator
pub static DYNAMIC_ALLOCATOR: RustAllocator = RustAllocator::empty();
#[alloc_error_handler]
/// alloc_error_handler function
fn alloc_error_handler(_: core::alloc::Layout) -> ! {
    panic!("alloc error handler panic!");
}

/// Critical Wrapped Buddy System Allocator
pub struct RustAllocator {
    Buddy_System_Allocator: LockedHeap<32>,
}

unsafe impl GlobalAlloc for RustAllocator {
    //这个trait必须实现
    //这些方法定义了内存分配、释放、清零分配和重新分配的行为
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        vTaskEnterCritical();
        //线程安全保护
        let x = self.Buddy_System_Allocator.alloc(layout);
        vTaskExitCritical();
        x
    }

    unsafe fn dealloc(&self, ptr: *mut u8, layout: Layout) {
        vTaskEnterCritical();
        self.Buddy_System_Allocator.dealloc(ptr, layout);
        vTaskExitCritical();
    }
    unsafe fn alloc_zeroed(&self, layout: Layout) -> *mut u8 {
        vTaskEnterCritical();
        let x = self.Buddy_System_Allocator.alloc_zeroed(layout);
        vTaskExitCritical();
        x
    }
    unsafe fn realloc(&self, ptr: *mut u8, layout: Layout, new_size: usize) -> *mut u8 {
        vTaskEnterCritical();
        let x = self.Buddy_System_Allocator.realloc(ptr, layout, new_size);
        vTaskExitCritical();
        x
    }
}

impl RustAllocator {
    pub const fn empty() -> Self {
        RustAllocator {
            Buddy_System_Allocator: LockedHeap::<32>::empty(),
        }
    }
}
