#![no_std]
use crate::portable::{portBASE_TYPE, portTickType};
use crate::queue::{xQueueGenericCreate, xQueueGenericReceive, xQueueGenericSend, xQueueHandle};

// Constants
const semGIVE_BLOCK_TIME: portTickType = 0;
const semTAKE_BLOCK_TIME: portTickType = 0;

// Semaphore type definitions
pub type SemaphoreHandle_t = xQueueHandle;

// Binary semaphore functions
pub fn xSemaphoreCreateBinary() -> SemaphoreHandle_t {
    // Create a binary semaphore using a queue with length 1 and item size 0
    let xHandle = xQueueGenericCreate(1, 0, 0);

    if !xHandle.is_null() {
        // Initialize the semaphore as empty
        xQueueGenericSend(xHandle, core::ptr::null(), 0, 0);
    }

    xHandle
}

pub fn xSemaphoreTake(xSemaphore: SemaphoreHandle_t, xBlockTime: portTickType) -> portBASE_TYPE {
    // Take the semaphore by receiving from the queue
    xQueueGenericReceive(xSemaphore, core::ptr::null_mut(), xBlockTime, 0)
}

pub fn xSemaphoreGive(xSemaphore: SemaphoreHandle_t) -> portBASE_TYPE {
    // Give the semaphore by sending to the queue
    xQueueGenericSend(xSemaphore, core::ptr::null(), semGIVE_BLOCK_TIME, 0)
}

// Counting semaphore functions
pub fn xSemaphoreCreateCounting(
    uxMaxCount: portBASE_TYPE,
    uxInitialCount: portBASE_TYPE,
) -> SemaphoreHandle_t {
    // Create a counting semaphore using a queue
    let xHandle = xQueueGenericCreate(uxMaxCount, 0, 0);

    if !xHandle.is_null() {
        // Initialize the semaphore with the initial count
        for _ in 0..uxInitialCount {
            xQueueGenericSend(xHandle, core::ptr::null(), 0, 0);
        }
    }

    xHandle
}

// Mutex functions
pub fn xSemaphoreCreateMutex() -> SemaphoreHandle_t {
    // Create a mutex using a binary semaphore
    xSemaphoreCreateBinary()
}

pub fn xSemaphoreTakeRecursive(
    xMutex: SemaphoreHandle_t,
    xBlockTime: portTickType,
) -> portBASE_TYPE {
    // Take the mutex recursively
    xQueueGenericReceive(xMutex, core::ptr::null_mut(), xBlockTime, 0)
}

pub fn xSemaphoreGiveRecursive(xMutex: SemaphoreHandle_t) -> portBASE_TYPE {
    // Give the mutex recursively
    xQueueGenericSend(xMutex, core::ptr::null(), semGIVE_BLOCK_TIME, 0)
}

// ISR versions of semaphore functions
pub fn xSemaphoreTakeFromISR(
    xSemaphore: SemaphoreHandle_t,
    pxHigherPriorityTaskWoken: *mut portBASE_TYPE,
) -> portBASE_TYPE {
    // Take the semaphore from ISR
    xQueueGenericReceive(xSemaphore, core::ptr::null_mut(), 0, 0)
}

pub fn xSemaphoreGiveFromISR(
    xSemaphore: SemaphoreHandle_t,
    pxHigherPriorityTaskWoken: *mut portBASE_TYPE,
) -> portBASE_TYPE {
    // Give the semaphore from ISR
    xQueueGenericSend(xSemaphore, core::ptr::null(), 0, 0)
}

// Semaphore deletion
pub fn vSemaphoreDelete(xSemaphore: SemaphoreHandle_t) {
    // Delete the semaphore by deleting the underlying queue
    crate::queue::vQueueDelete(xSemaphore);
}
