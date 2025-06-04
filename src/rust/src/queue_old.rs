#![no_std]

mod coroutine;
mod create;
mod definition;
mod mutex;
mod receive;
mod registry;
mod semaphore;
mod send;

// Re-export types
pub use definition::{xQueueHandle, QueueDefinition, QueueItem, QueueRegistryItem};

// Re-export queue creation and deletion functions
pub use create::{vQueueDelete, xQueueCreate, xQueueGenericCreate, xQueueGenericReset};

// Re-export queue send and receive functions
pub use receive::{
    uxQueueMessagesWaiting, uxQueueMessagesWaitingFromISR, xQueueGenericReceive,
    xQueueIsQueueEmptyFromISR, xQueueIsQueueFullFromISR, xQueuePeek, xQueuePeekFromISR,
    xQueueReceive, xQueueReceiveFromISR,
};
pub use send::{xQueueGenericSend, xQueueGenericSendFromISR, xQueueSend, xQueueSendFromISR};

// Re-export mutex functions
pub use mutex::{
    xQueueCreateMutex, xQueueGetMutexHolder, xQueueGiveMutex, xQueueGiveMutexRecursive,
    xQueueTakeMutex, xQueueTakeMutexRecursive,
};

// Re-export registry functions
pub use registry::{pcQueueGetName, vQueueAddToRegistry, vQueueUnregisterQueue};

// Re-export coroutine functions
pub use coroutine::{
    vQueueWaitForMessageRestricted, xQueueAltGenericReceive, xQueueAltGenericSend, xQueueCRReceive,
    xQueueCRReceiveFromISR, xQueueCRSend, xQueueCRSendFromISR,
};

// Re-export semaphore functions
pub use semaphore::{
    xQueueCreateBinarySemaphore, xQueueCreateCountingSemaphore, xQueueSemaphoreGive,
    xQueueSemaphoreGiveFromISR, xQueueSemaphoreTake, xQueueSemaphoreTakeFromISR,
};

// Queue constants
pub const queueQUEUE_TYPE_BASE: u8 = 0;
pub const queueQUEUE_TYPE_SET: u8 = 0;
pub const queueQUEUE_TYPE_MUTEX: u8 = 1;
pub const queueQUEUE_TYPE_COUNTING_SEMAPHORE: u8 = 2;
pub const queueQUEUE_TYPE_BINARY_SEMAPHORE: u8 = 3;
pub const queueQUEUE_TYPE_RECURSIVE_MUTEX: u8 = 4;

pub const queueSEND_TO_BACK: portBASE_TYPE = 0;
pub const queueSEND_TO_FRONT: portBASE_TYPE = 1;
pub const queueOVERWRITE: portBASE_TYPE = 2;

// Queue locking constants
pub const queueLOCKED_UNMODIFIED: portBASE_TYPE = 0;
pub const queueUNLOCKED: portBASE_TYPE = -1;

// Queue registry size
pub const configQUEUE_REGISTRY_SIZE: usize = 8;

// Queue error codes
pub const errQUEUE_FULL: portBASE_TYPE = 0;
pub const errQUEUE_EMPTY: portBASE_TYPE = 0;
pub const errQUEUE_BLOCKED: portBASE_TYPE = 0;
pub const errQUEUE_YIELD: portBASE_TYPE = 0;

// Queue type flags
pub const queueQUEUE_IS_MUTEX: u8 = 0;
pub const queueQUEUE_IS_COUNTING_SEMAPHORE: u8 = 1;
pub const queueQUEUE_IS_BINARY_SEMAPHORE: u8 = 2;
pub const queueQUEUE_IS_RECURSIVE_MUTEX: u8 = 3;
