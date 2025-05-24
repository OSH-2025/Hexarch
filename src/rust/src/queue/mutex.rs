#![no_std]
use super::definition::xQueueHandle;
use crate::portable::{portBASE_TYPE, portTickType};
use crate::task::{taskENTER_CRITICAL, taskEXIT_CRITICAL, xTaskGetCurrentTaskHandle};
use core::ffi::c_void;

#[no_mangle]
pub extern "C" fn xQueueCreateMutex() -> xQueueHandle {
    xQueueGenericCreate(1, 0, super::queueQUEUE_TYPE_MUTEX)
}

#[no_mangle]
pub extern "C" fn xQueueTakeMutex(xMutex: xQueueHandle, xBlockTime: portTickType) -> portBASE_TYPE {
    xQueueGenericReceive(xMutex, core::ptr::null_mut(), xBlockTime, 0)
}

#[no_mangle]
pub extern "C" fn xQueueGiveMutex(xMutex: xQueueHandle) -> portBASE_TYPE {
    xQueueGenericSend(xMutex, core::ptr::null(), 0, super::queueSEND_TO_BACK)
}

#[no_mangle]
pub extern "C" fn xQueueTakeMutexRecursive(
    xMutex: xQueueHandle,
    xBlockTime: portTickType,
) -> portBASE_TYPE {
    let pxMutex = xMutex as *mut super::definition::QueueDefinition;
    let xReturn: portBASE_TYPE;

    unsafe {
        // Check if the mutex is already held by this task
        if (*pxMutex).pxMutexHolder == xTaskGetCurrentTaskHandle() as *mut c_void {
            // Increment the recursive call count
            (*pxMutex).uxRecursiveCallCount += 1;
            xReturn = 1; // pdPASS
        } else {
            // Take the mutex
            xReturn = xQueueGenericReceive(xMutex, core::ptr::null_mut(), xBlockTime, 0);

            if xReturn == 1 {
                // Set the mutex holder to this task
                (*pxMutex).pxMutexHolder = xTaskGetCurrentTaskHandle() as *mut c_void;
                (*pxMutex).uxRecursiveCallCount = 1;
            }
        }
    }

    xReturn
}

#[no_mangle]
pub extern "C" fn xQueueGiveMutexRecursive(xMutex: xQueueHandle) -> portBASE_TYPE {
    let pxMutex = xMutex as *mut super::definition::QueueDefinition;
    let xReturn: portBASE_TYPE;

    unsafe {
        // Check if the mutex is held by this task
        if (*pxMutex).pxMutexHolder == xTaskGetCurrentTaskHandle() as *mut c_void {
            // Decrement the recursive call count
            (*pxMutex).uxRecursiveCallCount -= 1;

            // If this was the last recursive call, release the mutex
            if (*pxMutex).uxRecursiveCallCount == 0 {
                (*pxMutex).pxMutexHolder = core::ptr::null_mut();
                xReturn = xQueueGenericSend(xMutex, core::ptr::null(), 0, super::queueSEND_TO_BACK);
            } else {
                xReturn = 1; // pdPASS
            }
        } else {
            // The mutex is not held by this task
            xReturn = 0; // pdFAIL
        }
    }

    xReturn
}

#[no_mangle]
pub extern "C" fn xQueueGetMutexHolder(xMutex: xQueueHandle) -> *mut c_void {
    let pxMutex = xMutex as *mut super::definition::QueueDefinition;
    let xReturn: *mut c_void;

    unsafe {
        if (*pxMutex).ucQueueType == super::queueQUEUE_TYPE_MUTEX {
            xReturn = (*pxMutex).pxMutexHolder;
        } else {
            xReturn = core::ptr::null_mut();
        }
    }

    xReturn
}
