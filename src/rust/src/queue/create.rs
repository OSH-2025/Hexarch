#![no_std]
use super::*;
use super::{xQueueHandle, QueueDefinition};
use crate::list::vListInitialise;
use crate::portable::{portBASE_TYPE, pvPortMalloc, vPortFree};
use crate::task::{taskENTER_CRITICAL, taskEXIT_CRITICAL};
use core::ffi::c_void;

#[no_mangle]
pub extern "C" fn xQueueCreate(
    uxQueueLength: portBASE_TYPE,
    uxItemSize: portBASE_TYPE,
) -> xQueueHandle {
    xQueueGenericCreate(uxQueueLength, uxItemSize, super::queueQUEUE_TYPE_BASE)
}

#[no_mangle]
pub extern "C" fn xQueueGenericCreate(
    uxQueueLength: portBASE_TYPE,
    uxItemSize: portBASE_TYPE,
    ucQueueType: u8,
) -> xQueueHandle {
    let mut xReturn: xQueueHandle = core::ptr::null_mut();

    // Only create the queue if the queue length is greater than 0
    if uxQueueLength > 0 {
        // Allocate memory for the queue structure
        let pxNewQueue = unsafe {
            pvPortMalloc(core::mem::size_of::<QueueDefinition>()) as *mut QueueDefinition
        };

        if !pxNewQueue.is_null() {
            // Calculate the size of the queue storage area
            let xQueueSizeInBytes = (uxQueueLength * uxItemSize) as usize + 1;

            // Allocate memory for the queue storage area
            let pcHead = unsafe { pvPortMalloc(xQueueSizeInBytes) as *mut u8 };

            if !pcHead.is_null() {
                unsafe {
                    // Initialize the queue structure
                    (*pxNewQueue).uxLength = uxQueueLength;
                    (*pxNewQueue).uxItemSize = uxItemSize;
                    (*pxNewQueue).pcHead = pcHead;
                    (*pxNewQueue).pcTail = pcHead.add(xQueueSizeInBytes - 1);
                    (*pxNewQueue).ucQueueType = ucQueueType;
                    (*pxNewQueue).ucStaticallyAllocated = 0;
                    (*pxNewQueue).pvOwner = core::ptr::null_mut();
                    (*pxNewQueue).pxMutexHolder = core::ptr::null_mut();
                    (*pxNewQueue).uxRecursiveCallCount = 0;

                    // Reset the queue to its initial state
                    xQueueGenericReset(pxNewQueue as xQueueHandle, 1);
                }

                xReturn = pxNewQueue as xQueueHandle;
            } else {
                // Free the queue structure if storage allocation failed
                unsafe {
                    vPortFree(pxNewQueue as *mut c_void);
                }
            }
        }
    }

    xReturn
}

#[no_mangle]
pub extern "C" fn xQueueGenericReset(
    pxQueue: xQueueHandle,
    xNewQueue: portBASE_TYPE,
) -> portBASE_TYPE {
    let pxQueue = pxQueue as *mut QueueDefinition;

    unsafe {
        // Reset queue state
        (*pxQueue).uxMessagesWaiting = 0;
        (*pxQueue).pcWriteTo = (*pxQueue).pcHead;
        (*pxQueue).pcReadFrom = (*pxQueue)
            .pcHead
            .add(((*pxQueue).uxLength - 1) as usize * (*pxQueue).uxItemSize as usize);
        (*pxQueue).xRxLock = super::queueUNLOCKED;
        (*pxQueue).xTxLock = super::queueUNLOCKED;

        if xNewQueue == 0 {
            // Remove all tasks waiting on this queue
            while !crate::list::listLIST_IS_EMPTY(&mut (*pxQueue).xTasksWaitingToSend) {
                crate::task::xTaskRemoveFromEventList(&mut (*pxQueue).xTasksWaitingToSend);
            }
            while !crate::list::listLIST_IS_EMPTY(&mut (*pxQueue).xTasksWaitingToReceive) {
                crate::task::xTaskRemoveFromEventList(&mut (*pxQueue).xTasksWaitingToReceive);
            }
        } else {
            // Initialize the waiting lists
            vListInitialise(&mut (*pxQueue).xTasksWaitingToSend);
            vListInitialise(&mut (*pxQueue).xTasksWaitingToReceive);
        }
    }

    1 // pdPASS
}

#[no_mangle]
pub extern "C" fn vQueueDelete(pxQueue: xQueueHandle) {
    let pxQueue = pxQueue as *mut QueueDefinition;

    unsafe {
        // Remove all tasks waiting on this queue
        while !crate::list::listLIST_IS_EMPTY(&mut (*pxQueue).xTasksWaitingToSend) {
            crate::task::xTaskRemoveFromEventList(&mut (*pxQueue).xTasksWaitingToSend);
        }
        while !crate::list::listLIST_IS_EMPTY(&mut (*pxQueue).xTasksWaitingToReceive) {
            crate::task::xTaskRemoveFromEventList(&mut (*pxQueue).xTasksWaitingToReceive);
        }

        // Free the queue storage area
        if !(*pxQueue).pcHead.is_null() {
            vPortFree((*pxQueue).pcHead as *mut c_void);
        }

        // Free the queue structure
        vPortFree(pxQueue as *mut c_void);
    }
}
