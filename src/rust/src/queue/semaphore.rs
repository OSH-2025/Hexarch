#![no_std]
use super::*;
use super::xQueueHandle;
use crate::portable::{portBASE_TYPE, portTickType};
use core::ffi::c_void;

#[no_mangle]
pub extern "C" fn xQueueCreateCountingSemaphore(
    uxMaxCount: portBASE_TYPE,
    uxInitialCount: portBASE_TYPE,
) -> xQueueHandle {
    let xHandle = xQueueGenericCreate(uxMaxCount, 0, super::queueQUEUE_TYPE_COUNTING_SEMAPHORE);

    if !xHandle.is_null() {
        let pxQueue = xHandle as *mut super::QueueDefinition;
        unsafe {
            (*pxQueue).uxMessagesWaiting = uxInitialCount;
        }
    }

    xHandle
}

#[no_mangle]
pub extern "C" fn xQueueCreateBinarySemaphore() -> xQueueHandle {
    xQueueCreateCountingSemaphore(1, 1)
}

#[no_mangle]
pub extern "C" fn xQueueSemaphoreTake(
    xSemaphore: xQueueHandle,
    xTicksToWait: portTickType,
) -> portBASE_TYPE {
    xQueueGenericReceive(xSemaphore, core::ptr::null_mut(), xTicksToWait, 0)
}

#[no_mangle]
pub extern "C" fn xQueueSemaphoreGive(xSemaphore: xQueueHandle) -> portBASE_TYPE {
    xQueueGenericSend(xSemaphore, core::ptr::null(), 0, super::queueSEND_TO_BACK)
}

#[no_mangle]
pub extern "C" fn xQueueSemaphoreTakeFromISR(
    xSemaphore: xQueueHandle,
    pxHigherPriorityTaskWoken: *mut portBASE_TYPE,
) -> portBASE_TYPE {
    xQueueGenericReceiveFromISR(xSemaphore, core::ptr::null_mut(), pxHigherPriorityTaskWoken)
}

#[no_mangle]
pub extern "C" fn xQueueSemaphoreGiveFromISR(
    xSemaphore: xQueueHandle,
    pxHigherPriorityTaskWoken: *mut portBASE_TYPE,
) -> portBASE_TYPE {
    xQueueGenericSendFromISR(
        xSemaphore,
        core::ptr::null(),
        pxHigherPriorityTaskWoken,
        super::queueSEND_TO_BACK,
    )
}
