#![no_std]

// Re-export task module
pub mod task;

// Re-export commonly used types and functions
pub use task::{
    eNotifyAction,

    // Task state query
    eTaskGetState,
    pcTaskGetTaskName,
    // Types
    tskTCB,
    // Task runtime statistics
    ulTaskGetIdleRunTimeCounter,
    uxTaskGetNumberOfTasks,
    uxTaskGetTaskNumber,
    uxTaskPriorityGet,

    // Delay functions
    vTaskDelay,
    vTaskDelayUntil,

    vTaskDelete,

    vTaskEndScheduler,

    // Task event list management
    vTaskPlaceOnEventList,
    // Priority functions
    vTaskPrioritySet,
    vTaskResume,
    // Task hook functions
    vTaskSetApplicationTaskTag,
    vTaskSetTaskNumber,

    // Scheduler functions
    vTaskStartScheduler,
    // Task state management
    vTaskSuspend,
    xMemoryRegion,
    // Task abort delay
    xTaskAbortDelay,

    xTaskCallApplicationTaskHook,

    // Task creation and deletion
    xTaskCreate,
    xTaskGenericCreate,
    xTaskGetApplicationTaskTag,
    xTaskGetIdleTaskHandle,

    // Notification functions
    xTaskNotify,
    xTaskNotifyWait,
    xTaskRemoveFromEventList,

    xTaskResumeFromISR,
};

// Type definitions
pub type xTaskHandle = *mut task::tcb::tskTCB;
pub type pdTASK_CODE = Option<unsafe extern "C" fn(*mut core::ffi::c_void)>;
pub type pdTASK_HOOK_CODE =
    Option<unsafe extern "C" fn(*mut core::ffi::c_void) -> crate::portable::portBASE_TYPE>;

// Constants
pub const configMAX_TASK_NAME_LEN: usize = 16;
pub const configMAX_PRIORITIES: usize = 32;
pub const tskIDLE_PRIORITY: crate::portable::unsigned_portBASE_TYPE = 0;
pub const pdFALSE: crate::portable::portBASE_TYPE = 0;
pub const pdTRUE: crate::portable::portBASE_TYPE = 1;
pub const portMAX_DELAY: crate::portable::portTickType = crate::portable::portTickType::MAX;

// Task scheduler state constants
pub const taskSCHEDULER_NOT_STARTED: crate::portable::portBASE_TYPE = 0;
pub const taskSCHEDULER_RUNNING: crate::portable::portBASE_TYPE = 1;
pub const taskSCHEDULER_SUSPENDED: crate::portable::portBASE_TYPE = 2;
