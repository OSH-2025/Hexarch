#![no_std]

// Module declarations
pub mod delay;
pub mod notification;
pub mod priority;
pub mod scheduler;
pub mod tcb;

// Re-export commonly used types and functions
pub use delay::{vTaskDelay, vTaskDelayUntil};
pub use notification::{xTaskNotify, xTaskNotifyWait};
pub use priority::{
    uxTaskPriorityGet, uxTaskPriorityGetFromISR, vTaskPriorityDisinherit, vTaskPriorityInherit,
    vTaskPrioritySet,
};
pub use scheduler::{
    eTaskGetState, ulTaskGetIdleRunTimeCounter, uxTaskGetNumberOfTasks, vTaskEndScheduler,
    vTaskGetRunTimeStats, vTaskList, vTaskMissedYield, vTaskPlaceOnEventList,
    vTaskPlaceOnEventListRestricted, vTaskResume, vTaskSetTimeOutState, vTaskStartScheduler,
    vTaskSuspend, vTaskSuspendAll, xTaskAbortDelay, xTaskCheckForTimeOut,
    xTaskGetCurrentTaskHandle, xTaskGetIdleTaskHandle, xTaskGetSchedulerState,
    xTaskRemoveFromEventList, xTaskResumeAll, xTaskResumeFromISR,
};
pub use tcb::{
    pcTaskGetTaskName, tskTCB, uxTaskGetTaskNumber, vTaskDelete, vTaskSetApplicationTaskTag,
    vTaskSetTaskNumber, xMemoryRegion, xTaskCallApplicationTaskHook, xTaskCreate,
    xTaskGenericCreate, xTaskGetApplicationTaskTag,
};

// Type definitions
pub type xTaskHandle = *mut tskTCB;
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

// External function declarations
extern "C" {
    fn vApplicationStackOverflowHook(pxTask: xTaskHandle, pcTaskName: *mut i8);
    fn vApplicationTickHook();
    fn vApplicationIdleHook();
}
