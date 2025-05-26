#![no_std]

// Re-export submodules
pub mod coroutine;
pub mod create;
pub mod definition;
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
pub use definition::{xQueueHandle, QueueDefinition, QueueItem, QueueRegistryItem};
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
