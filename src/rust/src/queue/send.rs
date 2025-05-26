#![no_std]
use super::definition::{xQueueHandle, QueueDefinition};
use crate::list::listLIST_IS_EMPTY;
use crate::portable::{portBASE_TYPE, portTickType};
use crate::task::{
    taskENTER_CRITICAL, taskEXIT_CRITICAL, taskYIELD_IF_USING_PREEMPTION, vTaskPlaceOnEventList,
    vTaskSuspendAll, xTaskCheckForTimeOut, xTaskResumeAll,
};
use core::ffi::c_void;
use core::intrinsics::copy_nonoverlapping;

fn prvCopyDataToQueue(
    pxQueue: *mut QueueDefinition,
    pvItemToQueue: *const c_void,
    xPosition: portBASE_TYPE,
) {
    unsafe {
        if (*pxQueue).uxItemSize > 0 {
            let mut pcWriteTo = (*pxQueue).pcWriteTo;

            if xPosition == super::queueSEND_TO_BACK {
                // Copy data to the back of the queue
                copy_nonoverlapping(
                    pvItemToQueue as *const u8,
                    pcWriteTo,
                    (*pxQueue).uxItemSize as usize,
                );

                // Update the write pointer
                pcWriteTo = pcWriteTo.add((*pxQueue).uxItemSize as usize);
                if pcWriteTo
                    >= (*pxQueue)
                        .pcHead
                        .add((*pxQueue).uxLength as usize * (*pxQueue).uxItemSize as usize)
                {
                    pcWriteTo = (*pxQueue).pcHead;
                }
                (*pxQueue).pcWriteTo = pcWriteTo;
            } else {
                // Copy data to the front of the queue
                copy_nonoverlapping(
                    pvItemToQueue as *const u8,
                    (*pxQueue).pcReadFrom,
                    (*pxQueue).uxItemSize as usize,
                );

                // Update the read pointer
                (*pxQueue).pcReadFrom = (*pxQueue).pcReadFrom.sub((*pxQueue).uxItemSize as usize);
                if (*pxQueue).pcReadFrom < (*pxQueue).pcHead {
                    (*pxQueue).pcReadFrom = (*pxQueue)
                        .pcHead
                        .add(((*pxQueue).uxLength - 1) as usize * (*pxQueue).uxItemSize as usize);
                }
            }
        }

        (*pxQueue).uxMessagesWaiting += 1;
    }
}

#[no_mangle]
pub extern "C" fn xQueueSend(
    pxQueue: xQueueHandle,
    pvItemToQueue: *const c_void,
    xTicksToWait: portTickType,
) -> portBASE_TYPE {
    xQueueGenericSend(
        pxQueue,
        pvItemToQueue,
        xTicksToWait,
        super::queueSEND_TO_BACK,
    )
}

#[no_mangle]
pub extern "C" fn xQueueGenericSend(
    pxQueue: xQueueHandle,
    pvItemToQueue: *const c_void,
    xTicksToWait: portTickType,
    xCopyPosition: portBASE_TYPE,
) -> portBASE_TYPE {
    let pxQueue = pxQueue as *mut QueueDefinition;
    let mut xReturn = 0;
    let mut xEntryTimeSet = 0;
    let mut xTimeToWake = 0;

    unsafe {
        // Check if the queue is full
        if (*pxQueue).uxMessagesWaiting >= (*pxQueue).uxLength {
            if xTicksToWait == 0 {
                // Return immediately if no wait time specified
                xReturn = 0; // pdFAIL
            } else {
                // Add the task to the waiting list
                vTaskPlaceOnEventList(&mut (*pxQueue).xTasksWaitingToSend, xTicksToWait);

                // Yield to another task
                taskYIELD_IF_USING_PREEMPTION();

                // Check if the task was woken by a timeout
                if xTaskCheckForTimeOut(&mut xTimeToWake, &mut xEntryTimeSet) != 0 {
                    xReturn = 0; // pdFAIL
                } else {
                    // Try to send the item again
                    xReturn =
                        xQueueGenericSend(pxQueue as xQueueHandle, pvItemToQueue, 0, xCopyPosition);
                }
            }
        } else {
            // Copy the item to the queue
            match xCopyPosition {
                super::queueSEND_TO_BACK => {
                    // Copy to the back of the queue
                    core::ptr::copy_nonoverlapping(
                        pvItemToQueue as *const u8,
                        (*pxQueue).pcWriteTo,
                        (*pxQueue).uxItemSize as usize,
                    );
                    (*pxQueue).pcWriteTo = (*pxQueue).pcWriteTo.add((*pxQueue).uxItemSize as usize);
                    if (*pxQueue).pcWriteTo >= (*pxQueue).pcTail {
                        (*pxQueue).pcWriteTo = (*pxQueue).pcHead;
                    }
                }
                super::queueSEND_TO_FRONT => {
                    // Copy to the front of the queue
                    (*pxQueue).pcReadFrom =
                        (*pxQueue).pcReadFrom.sub((*pxQueue).uxItemSize as usize);
                    if (*pxQueue).pcReadFrom < (*pxQueue).pcHead {
                        (*pxQueue).pcReadFrom =
                            (*pxQueue).pcTail.sub((*pxQueue).uxItemSize as usize);
                    }
                    core::ptr::copy_nonoverlapping(
                        pvItemToQueue as *const u8,
                        (*pxQueue).pcReadFrom,
                        (*pxQueue).uxItemSize as usize,
                    );
                }
                _ => {
                    // Invalid copy position
                    xReturn = 0; // pdFAIL
                }
            }

            // Increment the message count
            (*pxQueue).uxMessagesWaiting += 1;

            // Check if any tasks are waiting to receive
            if !crate::list::listLIST_IS_EMPTY(&mut (*pxQueue).xTasksWaitingToReceive) {
                // Remove the first task from the waiting list
                let pxTCB = crate::list::listGET_OWNER_OF_HEAD_ENTRY(
                    &mut (*pxQueue).xTasksWaitingToReceive,
                );
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
pub extern "C" fn xQueueSendFromISR(
    pxQueue: xQueueHandle,
    pvItemToQueue: *const c_void,
    pxHigherPriorityTaskWoken: *mut portBASE_TYPE,
) -> portBASE_TYPE {
    xQueueGenericSendFromISR(
        pxQueue,
        pvItemToQueue,
        pxHigherPriorityTaskWoken,
        super::queueSEND_TO_BACK,
    )
}

#[no_mangle]
pub extern "C" fn xQueueGenericSendFromISR(
    pxQueue: xQueueHandle,
    pvItemToQueue: *const c_void,
    pxHigherPriorityTaskWoken: *mut portBASE_TYPE,
    xCopyPosition: portBASE_TYPE,
) -> portBASE_TYPE {
    let pxQueue = pxQueue as *mut QueueDefinition;
    let mut xReturn = 0;

    unsafe {
        // Check if the queue is full
        if (*pxQueue).uxMessagesWaiting >= (*pxQueue).uxLength {
            xReturn = 0; // pdFAIL
        } else {
            // Copy the item to the queue
            match xCopyPosition {
                super::queueSEND_TO_BACK => {
                    // Copy to the back of the queue
                    core::ptr::copy_nonoverlapping(
                        pvItemToQueue as *const u8,
                        (*pxQueue).pcWriteTo,
                        (*pxQueue).uxItemSize as usize,
                    );
                    (*pxQueue).pcWriteTo = (*pxQueue).pcWriteTo.add((*pxQueue).uxItemSize as usize);
                    if (*pxQueue).pcWriteTo >= (*pxQueue).pcTail {
                        (*pxQueue).pcWriteTo = (*pxQueue).pcHead;
                    }
                }
                super::queueSEND_TO_FRONT => {
                    // Copy to the front of the queue
                    (*pxQueue).pcReadFrom =
                        (*pxQueue).pcReadFrom.sub((*pxQueue).uxItemSize as usize);
                    if (*pxQueue).pcReadFrom < (*pxQueue).pcHead {
                        (*pxQueue).pcReadFrom =
                            (*pxQueue).pcTail.sub((*pxQueue).uxItemSize as usize);
                    }
                    core::ptr::copy_nonoverlapping(
                        pvItemToQueue as *const u8,
                        (*pxQueue).pcReadFrom,
                        (*pxQueue).uxItemSize as usize,
                    );
                }
                _ => {
                    // Invalid copy position
                    xReturn = 0; // pdFAIL
                }
            }

            // Increment the message count
            (*pxQueue).uxMessagesWaiting += 1;

            // Check if any tasks are waiting to receive
            if !crate::list::listLIST_IS_EMPTY(&mut (*pxQueue).xTasksWaitingToReceive) {
                // Remove the first task from the waiting list
                let pxTCB = crate::list::listGET_OWNER_OF_HEAD_ENTRY(
                    &mut (*pxQueue).xTasksWaitingToReceive,
                );
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
