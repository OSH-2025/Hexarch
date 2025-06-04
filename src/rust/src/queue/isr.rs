#![no_std]
use super::*;
use super::{xQueueHandle, QueueDefinition};
use crate::list::listLIST_IS_EMPTY;
use crate::portable::{
    portBASE_TYPE, portCLEAR_INTERRUPT_MASK_FROM_ISR, portSET_INTERRUPT_MASK_FROM_ISR,
};
use core::ffi::c_void;
use core::intrinsics::copy_nonoverlapping;

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
        // Check if there is space in the queue
        if (*pxQueue).uxMessagesWaiting < (*pxQueue).uxLength {
            // Copy the data to the queue
            if (*pxQueue).uxItemSize > 0 {
                let mut pcWriteTo = (*pxQueue).pcWriteTo;

                if xCopyPosition == super::queueSEND_TO_BACK {
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
                    (*pxQueue).pcReadFrom =
                        (*pxQueue).pcReadFrom.sub((*pxQueue).uxItemSize as usize);
                    if (*pxQueue).pcReadFrom < (*pxQueue).pcHead {
                        (*pxQueue).pcReadFrom = (*pxQueue).pcHead.add(
                            ((*pxQueue).uxLength - 1) as usize * (*pxQueue).uxItemSize as usize,
                        );
                    }
                }
            }

            (*pxQueue).uxMessagesWaiting += 1;

            // Check if there are tasks waiting to receive
            if !listLIST_IS_EMPTY(&(*pxQueue).xQueue) {
                if crate::task::xTaskRemoveFromEventList(&(*pxQueue).xQueue) != 0 {
                    if !pxHigherPriorityTaskWoken.is_null() {
                        *pxHigherPriorityTaskWoken = 1;
                    }
                }
            }

            xReturn = 1; // pdPASS
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
pub extern "C" fn xQueueReceiveFromISR(
    pxQueue: xQueueHandle,
    pvBuffer: *mut c_void,
    pxHigherPriorityTaskWoken: *mut portBASE_TYPE,
) -> portBASE_TYPE {
    let pxQueue = pxQueue as *mut QueueDefinition;
    let mut xReturn = 0;

    unsafe {
        // Check if there is data in the queue
        if (*pxQueue).uxMessagesWaiting > 0 {
            // Copy the data from the queue
            if (*pxQueue).uxItemSize > 0 {
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

            // Check if there are tasks waiting to send
            if !listLIST_IS_EMPTY(&(*pxQueue).xQueue) {
                if crate::task::xTaskRemoveFromEventList(&(*pxQueue).xQueue) != 0 {
                    if !pxHigherPriorityTaskWoken.is_null() {
                        *pxHigherPriorityTaskWoken = 1;
                    }
                }
            }

            xReturn = 1; // pdPASS
        }
    }

    xReturn
}

#[no_mangle]
pub extern "C" fn xQueuePeekFromISR(pxQueue: xQueueHandle, pvBuffer: *mut c_void) -> portBASE_TYPE {
    let pxQueue = pxQueue as *mut QueueDefinition;
    let mut xReturn = 0;

    unsafe {
        // Check if there is data in the queue
        if (*pxQueue).uxMessagesWaiting > 0 {
            // Copy the data from the queue
            if (*pxQueue).uxItemSize > 0 {
                copy_nonoverlapping(
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
pub extern "C" fn xQueueIsQueueEmptyFromISR(pxQueue: xQueueHandle) -> portBASE_TYPE {
    let pxQueue = pxQueue as *mut QueueDefinition;
    unsafe { (*pxQueue).uxMessagesWaiting == 0 }
}

#[no_mangle]
pub extern "C" fn xQueueIsQueueFullFromISR(pxQueue: xQueueHandle) -> portBASE_TYPE {
    let pxQueue = pxQueue as *mut QueueDefinition;
    unsafe { (*pxQueue).uxMessagesWaiting >= (*pxQueue).uxLength }
}
