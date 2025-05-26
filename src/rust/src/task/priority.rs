#![no_std]
use super::tcb::tskTCB;
use crate::list::xList;
use crate::portable::unsigned_portBASE_TYPE;

// Constants
const configMAX_PRIORITIES: unsigned_portBASE_TYPE = 32;
const tskIDLE_PRIORITY: unsigned_portBASE_TYPE = 0;
const tskNO_AFFINITY: unsigned_portBASE_TYPE = 0xFFFFFFFF;

// Task priority inheritance constants
const configUSE_MUTEXES: bool = true;
const configUSE_RECURSIVE_MUTEXES: bool = true;
const configUSE_COUNTING_SEMAPHORES: bool = true;

// Global variables
static mut xReadyTasksLists: [xList; configMAX_PRIORITIES as usize] =
    [xList::new(); configMAX_PRIORITIES as usize];
static mut uxTopReadyPriority: unsigned_portBASE_TYPE = 0;

pub fn vTaskPrioritySet(xTask: super::xTaskHandle, uxNewPriority: unsigned_portBASE_TYPE) {
    unsafe {
        let pxTCB = super::tcb::prvGetTCBFromHandle(xTask);

        if pxTCB != core::ptr::null_mut() {
            // Ensure priority is within valid range
            let uxNewPriority = if uxNewPriority >= configMAX_PRIORITIES {
                configMAX_PRIORITIES - 1
            } else {
                uxNewPriority
            };

            // Remove task from ready list
            if (*pxTCB).xGenericListItem.pxContainer != core::ptr::null_mut() {
                crate::list::vListRemove(&mut (*pxTCB).xGenericListItem);
            }

            // Update task priority
            (*pxTCB).uxPriority = uxNewPriority;

            // Add task back to ready list
            (*pxTCB).xGenericListItem.xItemValue = uxNewPriority;
            crate::list::vListInsert(
                &mut xReadyTasksLists[uxNewPriority as usize],
                &mut (*pxTCB).xGenericListItem,
            );

            // Force context switch if needed
            if pxTCB == super::tcb::prvGetTCBFromHandle(core::ptr::null_mut()) {
                crate::portable::portYIELD_WITHIN_API();
            }
        }
    }
}

pub fn uxTaskPriorityGet(xTask: super::xTaskHandle) -> unsigned_portBASE_TYPE {
    unsafe {
        let pxTCB = super::tcb::prvGetTCBFromHandle(xTask);
        if pxTCB != core::ptr::null_mut() {
            (*pxTCB).uxPriority
        } else {
            0
        }
    }
}

pub fn uxTaskPriorityGetFromISR(xTask: super::xTaskHandle) -> unsigned_portBASE_TYPE {
    unsafe {
        let pxTCB = super::tcb::prvGetTCBFromHandle(xTask);
        if pxTCB != core::ptr::null_mut() {
            (*pxTCB).uxPriority
        } else {
            0
        }
    }
}

pub fn vTaskPriorityInherit(pxMutexHolder: *mut tskTCB) {
    unsafe {
        if pxMutexHolder != core::ptr::null_mut() {
            let pxCurrentTCB = super::tcb::prvGetTCBFromHandle(core::ptr::null_mut());
            if (*pxMutexHolder).uxPriority < (*pxCurrentTCB).uxPriority {
                // Remove task from ready list
                if (*pxMutexHolder).xGenericListItem.pxContainer != core::ptr::null_mut() {
                    crate::list::vListRemove(&mut (*pxMutexHolder).xGenericListItem);
                }

                // Update task priority
                (*pxMutexHolder).uxPriority = (*pxCurrentTCB).uxPriority;

                // Add task back to ready list
                (*pxMutexHolder).xGenericListItem.xItemValue = (*pxCurrentTCB).uxPriority;
                crate::list::vListInsert(
       TasksLists[(*pxCurrentTCB).uxPriority as usize],
                    &mut (*pxMutexHolder).xGenericListItem,
                );

                // Force context switch if needed
                if pxMutexHolder == pxCurrentTCB {
                    crate::portable::portYIELD_WITHIN_API();
                }
            }
        }
    }
}

pub fn vTaskPriorityDisinherit(pxMutexHolder: *mut tskTCB) {
    unsafe {
        if pxMutexHolder != core::ptr::null_mut() {
            // Remove task from ready list
            if (*pxMutexHolder).xGenericListItem.pxContainer != core::ptr::null_mut() {
                crate::list::vListRemove(&mut (*pxMutexHolder).xGenericListItem);
            }

            // Restore original priority
            (*pxMutexHolder).uxPriority = (*pxMutexHolder).uxBasePriority;

            // Add task back to ready list
            (*pxMutexHolder).xGenericListItem.xItemValue = (*pxMutexHolder).uxBasePriority;
            crate::list::vListInsert(
                &mut xReadyTasksLists[(*pxMutexHolder).uxBasePriority as usize],
                &mut (*pxMutexHolder).xGenericListItem,
            );

            // Force context switch if needed
            if pxMutexHolder == super::tcb::prvGetTCBFromHandle(core::ptr::null_mut()) {
                crate::portable::portYIELD_WITHIN_API();
            }
        }
    }
}

extern "C" {
    fn portYIELD_WITHIN_API();
}
