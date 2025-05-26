#![no_std]
use crate::list::xList;
use crate::portable::{portBASE_TYPE, portTickType};
use core::ffi::c_void;

// Queue handle type
pub type xQueueHandle = *mut c_void;

// Queue item structure
#[repr(C)]
pub struct QueueItem {
    pub xItemValue: portTickType,
    pub pxNext: *mut QueueItem,
    pub pxPrevious: *mut QueueItem,
    pub pvOwner: *mut c_void,
    pub pvContainer: *mut c_void,
}

// Queue registry item structure
#[repr(C)]
pub struct QueueRegistryItem {
    pub pcQueueName: *mut i8,
    pub xHandle: xQueueHandle,
}

// Queue definition structure
#[repr(C)]
pub struct QueueDefinition {
    // Queue storage area
    pub pcHead: *mut u8,
    pub pcTail: *mut u8,
    pub pcWriteTo: *mut u8,
    pub pcReadFrom: *mut u8,

    // Queue waiting lists
    pub xTasksWaitingToSend: xList,
    pub xTasksWaitingToReceive: xList,

    // Queue state
    pub uxMessagesWaiting: portBASE_TYPE,
    pub uxLength: portBASE_TYPE,
    pub uxItemSize: portBASE_TYPE,
    pub xRxLock: portBASE_TYPE,
    pub xTxLock: portBASE_TYPE,

    // Queue type and allocation info
    pub ucQueueType: u8,
    pub ucStaticallyAllocated: u8,

    // Owner and mutex specific fields
    pub pvOwner: *mut c_void,
    pub pxMutexHolder: *mut c_void,
    pub uxRecursiveCallCount: portBASE_TYPE,
}

// Queue locking functions
pub fn prvLockQueue(pxQueue: *mut QueueDefinition) {
    unsafe {
        (*pxQueue).xRxLock = -1; // queueUNLOCKED
        (*pxQueue).xTxLock = -1; // queueUNLOCKED
    }
}

pub fn prvUnlockQueue(pxQueue: *mut QueueDefinition) {
    unsafe {
        (*pxQueue).xRxLock = 0; // queueLOCKED_UNMODIFIED
        (*pxQueue).xTxLock = 0; // queueLOCKED_UNMODIFIED
    }
}

// Queue state check functions
pub fn prvIsQueueEmpty(pxQueue: *mut QueueDefinition) -> portBASE_TYPE {
    unsafe { (*pxQueue).uxMessagesWaiting == 0 }
}

pub fn prvIsQueueFull(pxQueue: *mut QueueDefinition) -> portBASE_TYPE {
    unsafe { (*pxQueue).uxMessagesWaiting >= (*pxQueue).uxLength }
}
