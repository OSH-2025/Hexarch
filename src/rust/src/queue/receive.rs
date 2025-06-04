#![no_std]
use super::*;
use super::{xQueueHandle, QueueDefinition};
use crate::list::listLIST_IS_EMPTY;
use crate::portable::{portBASE_TYPE, portTickType};
use crate::task::{
    taskENTER_CRITICAL, taskEXIT_CRITICAL, taskYIELD_IF_USING_PREEMPTION, vTaskPlaceOnEventList,
    vTaskSuspendAll, xTaskCheckForTimeOut, xTaskResumeAll,
};
use core::ffi::c_void;
use core::intrinsics::copy_nonoverlapping;

fn prvCopyDataFromQueue(pxQueue: *mut QueueDefinition, pvBuffer: *mut c_void) {
    unsafe {
        if (*pxQueue).uxItemSize > 0 {
            // Copy data from the queue
            copy_nonoverlapping(
                (*pxQueue).pcReadFrom,
                pvBuffer as *mut u8,
                (*pxQueue).uxItemSize as usize,
            );

            // Update the read pointer
            (*pxQueue).pcReadFrom = (*pxQueue).pcReadFrom.add((*pxQueue).uxItemSize as usize);
            if (*pxQueue).pcReadFrom
                >= (*pxQueue)
                    .pcHead
                    .add((*pxQueue).uxLength as usize * (*pxQueue).uxItemSize as usize)
            {
                (*pxQueue).pcReadFrom = (*pxQueue).pcHead;
            }
        }

        (*pxQueue).uxMessagesWaiting -= 1;
    }
}

#[no_mangle]
pub extern "C" fn xQueueReceive(
    pxQueue: xQueueHandle,
    pvBuffer: *mut c_void,
    xTicksToWait: portTickType,
) -> portBASE_TYPE {
    xQueueGenericReceive(pxQueue, pvBuffer, xTicksToWait, 0)
}

#[no_mangle]
pub extern "C" fn xQueueGenericReceive(
    pxQueue: xQueueHandle,
    pvBuffer: *mut c_void,
    xTicksToWait: portTickType,
    xJustPeeking: portBASE_TYPE,
) -> portBASE_TYPE {
    let pxQueue = pxQueue as *mut QueueDefinition;
    let mut xReturn = 0;
    let mut xEntryTimeSet = 0;
    let mut xTimeToWake = 0;

    unsafe {
        // Check if the queue is empty
        if (*pxQueue).uxMessagesWaiting == 0 {
            if xTicksToWait == 0 {
                // Return immediately if no wait time specified
                xReturn = 0; // pdFAIL
            } else {
                // Add the task to the waiting list
                vTaskPlaceOnEventList(&mut (*pxQueue).xTasksWaitingToReceive, xTicksToWait);

                // Yield to another task
                taskYIELD_IF_USING_PREEMPTION();

                // Check if the task was woken by a timeout
                if xTaskCheckForTimeOut(&mut xTimeToWake, &mut xEntryTimeSet) != 0 {
                    xReturn = 0; // pdFAIL
                } else {
                    // Try to receive the item again
                    xReturn =
                        xQueueGenericReceive(pxQueue as xQueueHandle, pvBuffer, 0, xJustPeeking);
                }
            }
        } else {
            // Copy the item from the queue
            if !pvBuffer.is_null() {
                core::ptr::copy_nonoverlapping(
                    (*pxQueue).pcReadFrom,
                    pvBuffer as *mut u8,
                    (*pxQueue).uxItemSize as usize,
                );
            }

            // Update the read pointer if not just peeking
            if xJustPeeking == 0 {
                (*pxQueue).pcReadFrom = (*pxQueue).pcReadFrom.add((*pxQueue).uxItemSize as usize);
                if (*pxQueue).pcReadFrom >= (*pxQueue).pcTail {
                    (*pxQueue).pcReadFrom = (*pxQueue).pcHead;
                }

                // Decrement the message count
                (*pxQueue).uxMessagesWaiting -= 1;
            }

            // Check if any tasks are waiting to send
            if !crate::list::listLIST_IS_EMPTY(&mut (*pxQueue).xTasksWaitingToSend) {
                // Remove the first task from the waiting list
                let pxTCB =
                    crate::list::listGET_OWNER_OF_HEAD_ENTRY(&mut (*pxQueue).xTasksWaitingToSend);
                crate::list::uxListRemove(pxTCB);
                crate::task::vTaskInternalSetTimeOutState(&mut xTimeToWake);
                xReturn = 1; // pdPASS
            } else {
                xReturn = 1; // pdPASS
            }
        }
    }

    xReturn
}

#[no_mangle]
pub extern "C" fn xQueueReceiveFromISR(
    pxQueue: xQueueHandle,
    pvBuffer: *mut c_void,
    pxHigherPriorityTaskWoken: *mut portBASE_TYPE,
) -> portBASE_TYPE {
    let pxQueue = pxQueue as *mut QueueDefinition;
    let mut xReturn = 0;

    unsafe {
        // Check if the queue is empty
        if (*pxQueue).uxMessagesWaiting == 0 {
            xReturn = 0; // pdFAIL
        } else {
            // Copy the item from the queue
            if !pvBuffer.is_null() {
                core::ptr::copy_nonoverlapping(
                    (*pxQueue).pcReadFrom,
                    pvBuffer as *mut u8,
                    (*pxQueue).uxItemSize as usize,
                );
            }

            // Update the read pointer
            (*pxQueue).pcReadFrom = (*pxQueue).pcReadFrom.add((*pxQueue).uxItemSize as usize);
            if (*pxQueue).pcReadFrom >= (*pxQueue).pcTail {
                (*pxQueue).pcReadFrom = (*pxQueue).pcHead;
            }

            // Decrement the message count
            (*pxQueue).uxMessagesWaiting -= 1;

            // Check if any tasks are waiting to send
            if !crate::list::listLIST_IS_EMPTY(&mut (*pxQueue).xTasksWaitingToSend) {
                // Remove the first task from the waiting list
                let pxTCB =
                    crate::list::listGET_OWNER_OF_HEAD_ENTRY(&mut (*pxQueue).xTasksWaitingToSend);
                crate::list::uxListRemove(pxTCB);
                if pxHigherPriorityTaskWoken != core::ptr::null_mut() {
                    *pxHigherPriorityTaskWoken = 1;
                }
                xReturn = 1; // pdPASS
            } else {
                xReturn = 1; // pdPASS
            }
        }
    }

    xReturn
}

#[no_mangle]
pub extern "C" fn xQueuePeek(
    pxQueue: xQueueHandle,
    pvBuffer: *mut c_void,
    xTicksToWait: portTickType,
) -> portBASE_TYPE {
    xQueueGenericReceive(pxQueue, pvBuffer, xTicksToWait, 1)
}

#[no_mangle]
pub extern "C" fn xQueuePeekFromISR(pxQueue: xQueueHandle, pvBuffer: *mut c_void) -> portBASE_TYPE {
    let pxQueue = pxQueue as *mut QueueDefinition;
    let mut xReturn = 0;

    unsafe {
        // Check if the queue is empty
        if (*pxQueue).uxMessagesWaiting == 0 {
            xReturn = 0; // pdFAIL
        } else {
            // Copy the item from the queue
            if !pvBuffer.is_null() {
                core::ptr::copy_nonoverlapping(
                    (*pxQueue).pcReadFrom,
                    pvBuffer as *mut u8,
                    (*pxQueue).uxItemSize as usize,
                );
            }
            xReturn = 1; // pdPASS
        }
    }

    xReturn
}

#[no_mangle]
pub extern "C" fn uxQueueMessagesWaiting(pxQueue: xQueueHandle) -> portBASE_TYPE {
    let pxQueue = pxQueue as *mut QueueDefinition;
    let mut uxReturn = 0;

    unsafe {
        taskENTER_CRITICAL();
        uxReturn = (*pxQueue).uxMessagesWaiting;
        taskEXIT_CRITICAL();
    }

    uxReturn
}

#[no_mangle]
pub extern "C" fn uxQueueMessagesWaitingFromISR(pxQueue: xQueueHandle) -> portBASE_TYPE {
    let pxQueue = pxQueue as *mut QueueDefinition;
    unsafe { (*pxQueue).uxMessagesWaiting }
}

#[no_mangle]
pub extern "C" fn xQueueIsQueueEmptyFromISR(pxQueue: xQueueHandle) -> portBASE_TYPE {
    let pxQueue = pxQueue as *mut QueueDefinition;
    unsafe { (*pxQueue).uxMessagesWaiting == 0 }
}

#[no_mangle]
pub extern "C" fn xQueueIsQueueFullFromISR(pxQueue: xQueueHandle) -> portBASE_TYPE {
    let pxQueue = pxQueue as *mut QueueDefinition;
    unsafe { (*pxQueue).uxMessagesWaiting >= (*pxQueue).uxLength }
}
