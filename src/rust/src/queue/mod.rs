#![no_std]

// Re-export submodules
pub mod coroutine;
pub mod create;
// pub mod definition;
pub mod isr;
pub mod mutex;
pub mod receive;
pub mod registry;
pub mod semaphore;
pub mod send;

// Re-export commonly used types and functions
pub use coroutine::{
    vQueueWaitForMessageRestricted, xQueueAltGenericReceive, xQueueAltGenericSend, xQueueCRReceive,
    xQueueCRReceiveFromISR, xQueueCRSend, xQueueCRSendFromISR,
};
pub use create::{vQueueDelete, xQueueGenericCreate};
// pub use definition::{xQueueHandle, QueueDefinition, QueueItem, QueueRegistryItem};
pub use mutex::{xQueueCreateMutex, xQueueGiveMutexRecursive, xQueueTakeMutexRecursive};
pub use receive::{xQueueGenericReceive, xQueuePeek, xQueuePeekFromISR, xQueueReceiveFromISR};
pub use registry::{vQueueAddToRegistry, vQueueUnregisterQueue};
pub use semaphore::xQueueCreateCountingSemaphore;
pub use send::{xQueueGenericSend, xQueueSendFromISR};

// Queue type constants
pub const queueSEND_TO_BACK: i32 = 0;
pub const queueSEND_TO_FRONT: i32 = 1;

// Queue type identifiers
pub const queueQUEUE_TYPE_BASE: u8 = 0;
pub const queueQUEUE_TYPE_MUTEX: u8 = 1;
pub const queueQUEUE_TYPE_COUNTING_SEMAPHORE: u8 = 2;
pub const queueQUEUE_TYPE_BINARY_SEMAPHORE: u8 = 3;
pub const queueQUEUE_TYPE_RECURSIVE_MUTEX: u8 = 4;

// Queue type flags
pub const queueQUEUE_IS_MUTEX: u8 = 0;
pub const queueQUEUE_IS_COUNTING_SEMAPHORE: u8 = 1;
pub const queueQUEUE_IS_BINARY_SEMAPHORE: u8 = 2;
pub const queueQUEUE_IS_RECURSIVE_MUTEX: u8 = 3;

// Queue error codes
pub const errQUEUE_FULL: i32 = 0;
pub const errQUEUE_EMPTY: i32 = 0;
pub const errQUEUE_BLOCKED: i32 = 0;
pub const errQUEUE_YIELD: i32 = 0;

// Queue registry size
pub const configQUEUE_REGISTRY_SIZE: usize = 8;

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
