#![no_std]
use super::tcb::tskTCB;
use crate::list::xList;
use crate::portable::{portSTACK_TYPE, unsigned_portBASE_TYPE};
use core::ffi::c_void;

// Global variables
static mut pxCurrentTCB: *mut tskTCB = core::ptr::null_mut();
static mut uxSchedulerSuspended: unsigned_portBASE_TYPE = 0;
static mut xMissedYield: portBASE_TYPE = super::pdFALSE;
static mut uxMissedTicks: unsigned_portBASE_TYPE = 0;

// Scheduler control functions
pub fn vTaskStartScheduler() {
    unsafe {
        // Initialize the scheduler
        uxSchedulerSuspended = 0;

        // Create the idle task
        // TODO: Implement idle task creation

        // Start the first task
        portRESTORE_CONTEXT();
    }
}

pub fn vTaskEndScheduler() {
    unsafe {
        // Disable interrupts
        portDISABLE_INTERRUPTS();

        // Set scheduler suspended flag
        uxSchedulerSuspended = 1;

        // TODO: Implement scheduler shutdown
    }
}

pub fn vTaskSuspendAll() {
    unsafe {
        uxSchedulerSuspended += 1;
    }
}

pub fn xTaskResumeAll() -> i32 {
    unsafe {
        if uxSchedulerSuspended > 0 {
            uxSchedulerSuspended -= 1;

            if uxSchedulerSuspended == 0 {
                // Check if we need to switch tasks
                if xTaskGetSchedulerState() == 2 {
                    // taskSCHEDULER_RUNNING
                    portYIELD_WITHIN_API();
                }
                return 1;
            }
        }
        0
    }
}

// Task state functions
pub fn xTaskGetSchedulerState() -> unsigned_portBASE_TYPE {
    unsafe {
        if uxSchedulerSuspended > 0 {
            1 // taskSCHEDULER_SUSPENDED
        } else if pxCurrentTCB.is_null() {
            0 // taskSCHEDULER_NOT_STARTED
        } else {
            2 // taskSCHEDULER_RUNNING
        }
    }
}

pub fn vTaskSetTimeOutState(xTimeOut: *mut xTimeOut) {
    unsafe {
        (*xTimeOut).xOverflowCount = xNumOfOverflows;
        (*xTimeOut).xTimeOnEntering = xTickCount;
    }
}

pub fn xTaskCheckForTimeOut(
    xTimeOut: *mut xTimeOut,
    xTicksToWait: *mut unsigned_portBASE_TYPE,
) -> i32 {
    unsafe {
        let xConstTickCount = xTickCount;
        let xShouldDelay = if (*xTimeOut).xOverflowCount == xNumOfOverflows {
            // No overflow occurred
            if xConstTickCount - (*xTimeOut).xTimeOnEntering < *xTicksToWait {
                // Not enough time has passed
                *xTicksToWait -= xConstTickCount - (*xTimeOut).xTimeOnEntering;
                vTaskSetTimeOutState(xTimeOut);
                0
            } else {
                // Time has expired
                1
            }
        } else {
            // An overflow occurred
            if xConstTickCount - (*xTimeOut).xTimeOnEntering < *xTicksToWait {
                // Not enough time has passed
                *xTicksToWait -= xConstTickCount - (*xTimeOut).xTimeOnEntering;
                vTaskSetTimeOutState(xTimeOut);
                0
            } else {
                // Time has expired
                1
            }
        };
        xShouldDelay
    }
}

// Task switching functions
pub fn vTaskSwitchContext() {
    unsafe {
        if uxSchedulerSuspended == 0 {
            // Find the highest priority task that is ready to run
            let pxTCB = prvGetNextTask();
            if pxTCB != pxCurrentTCB {
                pxCurrentTCB = pxTCB;
            }
        }
    }
}

fn prvGetNextTask() -> *mut tskTCB {
    unsafe {
        // Get the next task from the ready list
        let pxList = &xReadyTasksLists[uxTopReadyPriority];
        let pxTCB = (*pxList).pxIndex.pxNext.pvOwner as *mut tskTCB;
        pxTCB
    }
}

// External declarations
extern "C" {
    fn portRESTORE_CONTEXT();
    fn portDISABLE_INTERRUPTS();
    fn portYIELD_WITHIN_API();
}

// Global variables
static mut xTickCount: unsigned_portBASE_TYPE = 0;
static mut xNumOfOverflows: unsigned_portBASE_TYPE = 0;
static mut xReadyTasksLists: [xList; configMAX_PRIORITIES] = [xList::new(); configMAX_PRIORITIES];
static mut uxTopReadyPriority: unsigned_portBASE_TYPE = 0;

#[repr(C)]
pub struct xTimeOut {
    pub xOverflowCount: unsigned_portBASE_TYPE,
    pub xTimeOnEntering: unsigned_portBASE_TYPE,
}

const configMAX_PRIORITIES: usize = 32;

// Task state management
pub fn vTaskSuspend(xTaskToSuspend: super::xTaskHandle) {
    unsafe {
        let pxTCB = super::tcb::prvGetTCBFromHandle(xTaskToSuspend);

        // Remove task from ready list
        if (*pxTCB).xGenericListItem.pxContainer != core::ptr::null_mut() {
            crate::list::vListRemove(&mut (*pxTCB).xGenericListItem);
        }

        // Add task to suspended list
        (*pxTCB).xGenericListItem.xItemValue = (*pxTCB).uxPriority;
        crate::list::vListInsert(&mut xSuspendedTaskList, &mut (*pxTCB).xGenericListItem);

        // Force context switch if needed
        if pxTCB == super::tcb::prvGetTCBFromHandle(core::ptr::null_mut()) {
            crate::portable::portYIELD_WITHIN_API();
        }
    }
}

pub fn vTaskResume(xTaskToResume: super::xTaskHandle) {
    unsafe {
        let pxTCB = super::tcb::prvGetTCBFromHandle(xTaskToResume);

        // Remove task from suspended list
        if (*pxTCB).xGenericListItem.pxContainer != core::ptr::null_mut() {
            crate::list::vListRemove(&mut (*pxTCB).xGenericListItem);

            // Add task to ready list
            (*pxTCB).xGenericListItem.xItemValue = (*pxTCB).uxPriority;
            crate::list::vListInsert(
                &mut xReadyTasksLists[(*pxTCB).uxPriority as usize],
                &mut (*pxTCB).xGenericListItem,
            );

            // Force context switch if needed
            if (*pxTCB).uxPriority > uxTopReadyPriority {
                crate::portable::portYIELD_WITHIN_API();
            }
        }
    }
}

pub fn xTaskResumeFromISR(xTaskToResume: super::xTaskHandle) -> portSTACK_TYPE {
    unsafe {
        let pxTCB = super::tcb::prvGetTCBFromHandle(xTaskToResume);
        let mut xYieldRequired = super::pdFALSE;

        if (*pxTCB).xGenericListItem.pxContainer != core::ptr::null_mut() {
            crate::list::vListRemove(&mut (*pxTCB).xGenericListItem);

            (*pxTCB).xGenericListItem.xItemValue = (*pxTCB).uxPriority;
            crate::list::vListInsert(
                &mut xReadyTasksLists[(*pxTCB).uxPriority as usize],
                &mut (*pxTCB).xGenericListItem,
            );

            if (*pxTCB).uxPriority > uxTopReadyPriority {
                xYieldRequired = super::pdTRUE;
            }
        }

        xYieldRequired
    }
}

// Task state query
pub fn eTaskGetState(xTask: super::xTaskHandle) -> crate::portable::eTaskState {
    unsafe {
        let pxTCB = super::tcb::prvGetTCBFromHandle(xTask);
        if pxTCB == core::ptr::null_mut() {
            return crate::portable::eTaskState::eInvalid;
        }

        if pxTCB == super::tcb::prvGetTCBFromHandle(core::ptr::null_mut()) {
            crate::portable::eTaskState::eRunning
        } else if (*pxTCB).xGenericListItem.pxContainer == core::ptr::null_mut() {
            crate::portable::eTaskState::eDeleted
        } else if (*pxTCB).xGenericListItem.pxContainer == &mut xSuspendedTaskList {
            crate::portable::eTaskState::eSuspended
        } else {
            crate::portable::eTaskState::eReady
        }
    }
}

// Task information
pub fn uxTaskGetNumberOfTasks() -> unsigned_portBASE_TYPE {
    unsafe { uxCurrentNumberOfTasks }
}

// Task runtime statistics
pub fn ulTaskGetIdleRunTimeCounter() -> u32 {
    unsafe {
        let pxTCB = super::tcb::prvGetTCBFromHandle(xIdleTaskHandle);
        if pxTCB != core::ptr::null_mut() {
            (*pxTCB).ulRunTimeCounter
        } else {
            0
        }
    }
}

pub fn xTaskGetIdleTaskHandle() -> super::xTaskHandle {
    unsafe { xIdleTaskHandle }
}

// Task event list management
pub fn vTaskPlaceOnEventList(pxEventList: *mut xList, xTicksToWait: portSTACK_TYPE) {
    unsafe {
        let pxTCB = super::tcb::prvGetTCBFromHandle(core::ptr::null_mut());
        let xTimeToWake = xTickCount + xTicksToWait;

        (*pxTCB).xGenericListItem.xItemValue = xTimeToWake;
        crate::list::vListInsert(pxEventList, &mut (*pxTCB).xGenericListItem);

        if xTimeToWake < xNextTaskUnblockTime {
            xNextTaskUnblockTime = xTimeToWake;
        }
    }
}

pub fn xTaskRemoveFromEventList(pxEventList: *mut xList) -> portSTACK_TYPE {
    unsafe {
        let pxTCB = crate::list::listGET_OWNER_OF_HEAD_ENTRY(pxEventList) as *mut tskTCB;
        crate::list::vListRemove(&mut (*pxTCB).xGenericListItem);

        (*pxTCB).xGenericListItem.xItemValue = (*pxTCB).uxPriority;
        crate::list::vListInsert(
            &mut xReadyTasksLists[(*pxTCB).uxPriority as usize],
            &mut (*pxTCB).xGenericListItem,
        );

        if (*pxTCB).uxPriority > uxTopReadyPriority {
            super::pdTRUE
        } else {
            super::pdFALSE
        }
    }
}

// Task abort delay
pub fn xTaskAbortDelay(xTask: super::xTaskHandle) -> portSTACK_TYPE {
    unsafe {
        let pxTCB = super::tcb::prvGetTCBFromHandle(xTask);
        if pxTCB != core::ptr::null_mut() {
            if (*pxTCB).xGenericListItem.pxContainer != core::ptr::null_mut() {
                crate::list::vListRemove(&mut (*pxTCB).xGenericListItem);

                (*pxTCB).xGenericListItem.xItemValue = (*pxTCB).uxPriority;
                crate::list::vListInsert(
                    &mut xReadyTasksLists[(*pxTCB).uxPriority as usize],
                    &mut (*pxTCB).xGenericListItem,
                );

                if pxTCB == super::tcb::prvGetTCBFromHandle(core::ptr::null_mut()) {
                    crate::portable::portYIELD_WITHIN_API();
                }
                super::pdTRUE
            } else {
                super::pdFALSE
            }
        } else {
            super::pdFALSE
        }
    }
}

// Task list and statistics
pub fn vTaskList(pcWriteBuffer: *mut i8) {
    unsafe {
        let mut pxTCB: *mut tskTCB;
        let mut uxTask = 0;
        let mut pcTaskName: *const i8;
        let mut eState: crate::portable::eTaskState;
        let mut uxPriority: unsigned_portBASE_TYPE;
        let mut ulRunTime: u32;

        // Write header
        let header = b"Task          State   Prio    Stack    Num     Core\r\n";
        let mut i = 0;
        while i < header.len() {
            *pcWriteBuffer.add(i) = header[i] as i8;
            i += 1;
        }
        pcWriteBuffer = pcWriteBuffer.add(i);

        // Write task information
        pxTCB = prvGetNextTask();
        while !pxTCB.is_null() {
            pcTaskName = (*pxTCB).pcTaskName.as_ptr();
            eState = eTaskGetState(pxTCB);
            uxPriority = (*pxTCB).uxPriority;
            ulRunTime = (*pxTCB).ulRunTimeCounter;

            // Format task information
            let mut buffer = [0u8; 100];
            let len = core::fmt::write(
                &mut buffer.as_mut_slice(),
                format_args!(
                    "{:<12} {:<6} {:<7} {:<8} {:<7} {:<7}\r\n",
                    core::str::from_utf8_unchecked(core::slice::from_raw_parts(
                        pcTaskName as *const u8,
                        super::configMAX_TASK_NAME_LEN
                    )),
                    match eState {
                        crate::portable::eTaskState::eRunning => "Running",
                        crate::portable::eTaskState::eReady => "Ready",
                        crate::portable::eTaskState::eBlocked => "Blocked",
                        crate::portable::eTaskState::eSuspended => "Suspended",
                        crate::portable::eTaskState::eDeleted => "Deleted",
                        _ => "Invalid",
                    },
                    uxPriority,
                    (*pxTCB).pxEndOfStack.offset_from((*pxTCB).pxStack) as isize,
                    (*pxTCB).uxTaskNumber,
                    ulRunTime,
                ),
            )
            .unwrap_or(0);

            // Write to buffer
            i = 0;
            while i < len {
                *pcWriteBuffer.add(i) = buffer[i] as i8;
                i += 1;
            }
            pcWriteBuffer = pcWriteBuffer.add(i);

            uxTask += 1;
            pxTCB = prvGetNextTask();
        }
    }
}

pub fn xTaskGetCurrentTaskHandle() -> super::xTaskHandle {
    unsafe { pxCurrentTCB }
}

pub fn vTaskMissedYield() {
    unsafe {
        xMissedYield = super::pdTRUE;
    }
}

pub fn vTaskGetRunTimeStats(pcWriteBuffer: *mut i8) {
    unsafe {
        let mut pxTCB: *mut tskTCB;
        let mut uxTask = 0;
        let mut pcTaskName: *const i8;
        let mut ulRunTime: u32;
        let mut ulTotalRunTime: u32 = 0;

        // Calculate total run time
        pxTCB = prvGetNextTask();
        while !pxTCB.is_null() {
            ulTotalRunTime += (*pxTCB).ulRunTimeCounter;
            pxTCB = prvGetNextTask();
        }

        // Write header
        let header = b"Task          Abs Time       % Time\r\n";
        let mut i = 0;
        while i < header.len() {
            *pcWriteBuffer.add(i) = header[i] as i8;
            i += 1;
        }
        pcWriteBuffer = pcWriteBuffer.add(i);

        // Write task statistics
        pxTCB = prvGetNextTask();
        while !pxTCB.is_null() {
            pcTaskName = (*pxTCB).pcTaskName.as_ptr();
            ulRunTime = (*pxTCB).ulRunTimeCounter;

            // Format task statistics
            let mut buffer = [0u8; 100];
            let len = core::fmt::write(
                &mut buffer.as_mut_slice(),
                format_args!(
                    "{:<12} {:<14} {:<7}\r\n",
                    core::str::from_utf8_unchecked(core::slice::from_raw_parts(
                        pcTaskName as *const u8,
                        super::configMAX_TASK_NAME_LEN
                    )),
                    ulRunTime,
                    if ulTotalRunTime > 0 {
                        (ulRunTime * 100) / ulTotalRunTime
                    } else {
                        0
                    },
                ),
            )
            .unwrap_or(0);

            // Write to buffer
            i = 0;
            while i < len {
                *pcWriteBuffer.add(i) = buffer[i] as i8;
                i += 1;
            }
            pcWriteBuffer = pcWriteBuffer.add(i);

            uxTask += 1;
            pxTCB = prvGetNextTask();
        }
    }
}

pub fn vTaskPlaceOnEventListRestricted(
    pxEventList: *mut xList,
    xTicksToWait: portSTACK_TYPE,
    xWaitIndefinitely: portBASE_TYPE,
) {
    unsafe {
        let pxTCB = super::tcb::prvGetTCBFromHandle(core::ptr::null_mut());
        let xTimeToWake = if xWaitIndefinitely == super::pdTRUE {
            super::portMAX_DELAY
        } else {
            xTickCount + xTicksToWait
        };

        (*pxTCB).xGenericListItem.xItemValue = xTimeToWake;
        crate::list::vListInsert(pxEventList, &mut (*pxTCB).xGenericListItem);

        if xTimeToWake < xNextTaskUnblockTime {
            xNextTaskUnblockTime = xTimeToWake;
        }
    }
}
