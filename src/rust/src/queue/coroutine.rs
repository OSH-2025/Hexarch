#![no_std]
use super::*;
use crate::portable::{portBASE_TYPE, portTickType};
use crate::task::{taskENTER_CRITICAL, taskEXIT_CRITICAL};
use core::ffi::c_void;

#[no_mangle]
pub extern "C" fn xQueueAltGenericSend(
    pxQueue: xQueueHandle,
    pvItemToQueue: *const c_void,
    xTicksToWait: portTickType,
    xCopyPosition: portBASE_TYPE,
) -> portBASE_TYPE {
    let pxQueue = pxQueue as *mut super::QueueDefinition;
    let mut xReturn = 0;

    unsafe {
        taskENTER_CRITICAL();
        {
            // Check if the queue is full
            if (*pxQueue).uxMessagesWaiting >= (*pxQueue).uxLength {
                if xTicksToWait == 0 {
                    // Return immediately if no wait time specified
                    xReturn = 0; // pdFAIL
                } else {
                    // Add the task to the waiting list
                    crate::task::vTaskPlaceOnEventList(
                        &mut (*pxQueue).xTasksWaitingToSend,
                        xTicksToWait,
                    );
                    taskEXIT_CRITICAL();
                    crate::task::vTaskSuspendAll();
                    xReturn = xQueueAltGenericSend(
                        pxQueue as xQueueHandle,
                        pvItemToQueue,
                        0,
                        xCopyPosition,
                    );
                    crate::task::xTaskResumeAll();
                    return xReturn;
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
                        (*pxQueue).pcWriteTo =
                            (*pxQueue).pcWriteTo.add((*pxQueue).uxItemSize as usize);
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
                    xReturn = 1; // pdPASS
                } else {
                    xReturn = 1; // pdPASS
                }
            }
        }
        taskEXIT_CRITICAL();
    }

    xReturn
}

#[no_mangle]
pub extern "C" fn xQueueAltGenericReceive(
    pxQueue: xQueueHandle,
    pvBuffer: *mut c_void,
    xTicksToWait: portTickType,
    xJustPeeking: portBASE_TYPE,
) -> portBASE_TYPE {
    let pxQueue = pxQueue as *mut super::definition::QueueDefinition;
    let mut xReturn = 0;

    unsafe {
        taskENTER_CRITICAL();
        {
            // Check if the queue is empty
            if (*pxQueue).uxMessagesWaiting == 0 {
                if xTicksToWait == 0 {
                    // Return immediately if no wait time specified
                    xReturn = 0; // pdFAIL
                } else {
                    // Add the task to the waiting list
                    crate::task::vTaskPlaceOnEventList(
                        &mut (*pxQueue).xTasksWaitingToReceive,
                        xTicksToWait,
                    );
                    taskEXIT_CRITICAL();
                    crate::task::vTaskSuspendAll();
                    xReturn =
                        xQueueAltGenericReceive(pxQueue as xQueueHandle, pvBuffer, 0, xJustPeeking);
                    crate::task::xTaskResumeAll();
                    return xReturn;
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
                    (*pxQueue).pcReadFrom =
                        (*pxQueue).pcReadFrom.add((*pxQueue).uxItemSize as usize);
                    if (*pxQueue).pcReadFrom >= (*pxQueue).pcTail {
                        (*pxQueue).pcReadFrom = (*pxQueue).pcHead;
                    }

                    // Decrement the message count
                    (*pxQueue).uxMessagesWaiting -= 1;
                }

                // Check if any tasks are waiting to send
                if !crate::list::listLIST_IS_EMPTY(&mut (*pxQueue).xTasksWaitingToSend) {
                    // Remove the first task from the waiting list
                    let pxTCB = crate::list::listGET_OWNER_OF_HEAD_ENTRY(
                        &mut (*pxQueue).xTasksWaitingToSend,
                    );
                    crate::list::uxListRemove(pxTCB);
                    xReturn = 1; // pdPASS
                } else {
                    xReturn = 1; // pdPASS
                }
            }
        }
        taskEXIT_CRITICAL();
    }

    xReturn
}

#[no_mangle]
pub extern "C" fn vQueueWaitForMessageRestricted(
    pxQueue: xQueueHandle,
    xTicksToWait: portTickType,
    xWaitIndefinitely: portBASE_TYPE,
) {
    let pxQueue = pxQueue as *mut super::definition::QueueDefinition;

    unsafe {
        taskENTER_CRITICAL();
        {
            // Check if the queue is empty
            if (*pxQueue).uxMessagesWaiting == 0 {
                // Add the task to the waiting list
                if xWaitIndefinitely != 0 {
                    crate::task::vTaskPlaceOnEventList(
                        &mut (*pxQueue).xTasksWaitingToReceive,
                        xTicksToWait,
                    );
                } else {
                    crate::task::vTaskPlaceOnEventListRestricted(
                        &mut (*pxQueue).xTasksWaitingToReceive,
                        xTicksToWait,
                    );
                }
            }
        }
        taskEXIT_CRITICAL();
    }
}

#[no_mangle]
pub extern "C" fn xQueueCRSend(
    pxQueue: xQueueHandle,
    pvItemToQueue: *const c_void,
    xTicksToWait: portTickType,
) -> portBASE_TYPE {
    xQueueAltGenericSend(
        pxQueue,
        pvItemToQueue,
        xTicksToWait,
        super::queueSEND_TO_BACK,
    )
}

#[no_mangle]
pub extern "C" fn xQueueCRReceive(
    pxQueue: xQueueHandle,
    pvBuffer: *mut c_void,
    xTicksToWait: portTickType,
) -> portBASE_TYPE {
    xQueueAltGenericReceive(pxQueue, pvBuffer, xTicksToWait, 0)
}

#[no_mangle]
pub extern "C" fn xQueueCRSendFromISR(
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
pub extern "C" fn xQueueCRReceiveFromISR(
    pxQueue: xQueueHandle,
    pvBuffer: *mut c_void,
    pxHigherPriorityTaskWoken: *mut portBASE_TYPE,
) -> portBASE_TYPE {
    xQueueGenericReceiveFromISR(pxQueue, pvBuffer, pxHigherPriorityTaskWoken)
}
