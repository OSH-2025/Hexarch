#![no_std]
use super::scheduler::{vTaskSetTimeOutState, xTaskCheckForTimeOut, xTaskGetSchedulerState};
use super::tcb::tskTCB;
use crate::portable::unsigned_portBASE_TYPE;

// Global variables
static mut xTickCount: unsigned_portBASE_TYPE = 0;
static mut xDelayedTaskList: crate::list::xList = crate::list::xList::new();
static mut xNextTaskUnblockTime: unsigned_portBASE_TYPE = super::portMAX_DELAY;

pub fn vTaskDelay(xTicksToDelay: unsigned_portBASE_TYPE) {
    unsafe {
        if xTicksToDelay > 0 {
            if xTaskGetSchedulerState() != 2 {
                // taskSCHEDULER_RUNNING
                return;
            }

            // Get current task
            let pxTCB = super::tcb::prvGetTCBFromHandle(core::ptr::null_mut());

            // Remove from ready list
            crate::list::vListRemove(&mut (*pxTCB).xGenericListItem);

            // Add to delayed list
            let xTimeToWake = xTickCount + xTicksToDelay;
            (*pxTCB).xGenericListItem.xItemValue = xTimeToWake;
            crate::list::vListInsert(&mut xDelayedTaskList, &mut (*pxTCB).xGenericListItem);

            // Force context switch
            portYIELD_WITHIN_API();
        }
    }
}

pub fn vTaskDelayUntil(
    pxPreviousWakeTime: *mut unsigned_portBASE_TYPE,
    xTimeIncrement: unsigned_portBASE_TYPE,
) {
    unsafe {
        if xTaskGetSchedulerState() != 2 {
            // taskSCHEDULER_RUNNING
            return;
        }

        let xTimeToWake = *pxPreviousWakeTime + xTimeIncrement;
        let mut xShouldDelay = 0;

        // Check for overflow
        if xTickCount < *pxPreviousWakeTime {
            if xTimeToWake < *pxPreviousWakeTime && xTimeToWake > xTickCount {
                xShouldDelay = 1;
            }
        } else {
            if xTimeToWake < *pxPreviousWakeTime || xTimeToWake > xTickCount {
                xShouldDelay = 1;
            }
        }

        // Update wake time
        *pxPreviousWakeTime = xTimeToWake;

        if xShouldDelay != 0 {
            // Get current task
            let pxTCB = super::tcb::prvGetTCBFromHandle(core::ptr::null_mut());

            // Remove from ready list
            crate::list::vListRemove(&mut (*pxTCB).xGenericListItem);

            // Add to delayed list
            (*pxTCB).xGenericListItem.xItemValue = xTimeToWake;
            crate::list::vListInsert(&mut xDelayedTaskList, &mut (*pxTCB).xGenericListItem);

            // Force context switch
            portYIELD_WITHIN_API();
        }
    }
}

// Function to process delayed tasks
pub fn prvProcessDelayedTasks() {
    unsafe {
        let xConstTickCount = xTickCount;
        let mut pxTCB: *mut tskTCB;

        // Process all tasks that have expired
        while !xDelayedTaskList.xListEnd.pxNext.is_null() {
            pxTCB = xDelayedTaskList.xListEnd.pxNext.pvOwner as *mut tskTCB;

            if xConstTickCount < (*pxTCB).xGenericListItem.xItemValue {
                // Not time to wake this task yet
                break;
            }

            // Remove from delayed list
            crate::list::vListRemove(&mut (*pxTCB).xGenericListItem);

            // Add to ready list
            (*pxTCB).xGenericListItem.xItemValue = (*pxTCB).uxPriority;
            crate::list::vListInsert(
                &mut xReadyTasksLists[(*pxTCB).uxPriority as usize],
                &mut (*pxTCB).xGenericListItem,
            );
        }
    }
}

pub fn prvAddCurrentTaskToDelayedList(xTimeToWake: unsigned_portBASE_TYPE) {
    unsafe {
        // Remove the current task from the ready list
        crate::list::vListRemove(
            &mut (*super::tcb::prvGetTCBFromHandle(core::ptr::null_mut())).xGenericListItem,
        );

        // Set the wake time in the generic list item
        (*super::tcb::prvGetTCBFromHandle(core::ptr::null_mut()))
            .xGenericListItem
            .xItemValue = xTimeToWake;

        // Add the task to the delayed list
        if xTimeToWake < xTickCount {
            // The wake time has already passed, add to the overflow list
            crate::list::vListInsert(
                &mut xDelayedTaskList,
                &mut (*super::tcb::prvGetTCBFromHandle(core::ptr::null_mut())).xGenericListItem,
            );
        } else {
            // The wake time is in the future, add to the delayed list
            crate::list::vListInsert(
                &mut xDelayedTaskList,
                &mut (*super::tcb::prvGetTCBFromHandle(core::ptr::null_mut())).xGenericListItem,
            );

            // Update the next task unblock time if necessary
            if xTimeToWake < xNextTaskUnblockTime {
                xNextTaskUnblockTime = xTimeToWake;
            }
        }
    }
}

pub fn prvCheckDelayedTasks() {
    unsafe {
        if xTickCount >= xNextTaskUnblockTime {
            loop {
                if crate::list::listLIST_IS_EMPTY(&xDelayedTaskList) {
                    // No more delayed tasks
                    xNextTaskUnblockTime = super::portMAX_DELAY;
                    break;
                } else {
                    let pxTCB =
                        crate::list::listGET_OWNER_OF_HEAD_ENTRY(&xDelayedTaskList) as *mut tskTCB;
                    let xItemValue = (*pxTCB).xGenericListItem.xItemValue;

                    if xTickCount < xItemValue {
                        // The task is not ready to wake up yet
                        xNextTaskUnblockTime = xItemValue;
                        break;
                    }

                    // Remove the task from the delayed list
                    crate::list::vListRemove(&mut (*pxTCB).xGenericListItem);

                    // Add to ready list
                    (*pxTCB).xGenericListItem.xItemValue = (*pxTCB).uxPriority;
                    crate::list::vListInsert(
                        &mut super::scheduler::xReadyTasksLists[(*pxTCB).uxPriority as usize],
                        &mut (*pxTCB).xGenericListItem,
                    );
                }
            }
        }
    }
}

extern "C" {
    fn portYIELD_WITHIN_API();
}

// External variables
extern "C" {
    static mut xReadyTasksLists: [crate::list::xList; 32];
}
