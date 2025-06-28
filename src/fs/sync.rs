//! FatFs synchronization support for multi-threading

use alloc::boxed::Box; // 添加：导入 Box
use core::ffi::c_void;
use crate::kernel::semphr::*;
use crate::kernel::tasks::*;
use crate::kernel::queue::*;
// 删除或注释掉这一行：
// use crate::portable::portmacro::portMAX_DELAY;

// 可以使用一个常量替代 portMAX_DELAY
const MAX_DELAY: usize = 0xFFFFFFFF;

#[no_mangle]
pub extern "C" fn ff_mutex_create() -> *mut c_void {
    let mutex = xSemaphoreCreateBinary!();
    let boxed_mutex = Box::new(mutex);
    Box::into_raw(boxed_mutex) as *mut c_void
}

#[no_mangle]
pub extern "C" fn ff_mutex_delete(sobj: *mut c_void) {
    if !sobj.is_null() {
        let _ = unsafe { Box::from_raw(sobj as *mut SemaphoreHandle_t) };
    }
}

#[no_mangle]
pub extern "C" fn ff_mutex_request(sobj: *mut c_void) -> i32 {
    if sobj.is_null() {
        return 0;
    }
    let mutex = unsafe { &*(sobj as *const SemaphoreHandle_t) };
    if xSemaphoreTake!(mutex, MAX_DELAY) == 1 { // 使用 MAX_DELAY
        1
    } else {
        0
    }
}

#[no_mangle]
pub extern "C" fn ff_mutex_release(sobj: *mut c_void) {
    if !sobj.is_null() {
        let mutex = unsafe { &*(sobj as *const SemaphoreHandle_t) };
        xSemaphoreGive!(mutex);
    }
}