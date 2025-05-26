#![no_std]
use super::scheduler;
use super::{configMAX_TASK_NAME_LEN, pdTASK_HOOK_CODE};
use crate::list::xList;
use crate::portable::{portBASE_TYPE, portTickType};
use crate::portable::{portSTACK_TYPE, unsigned_portBASE_TYPE};
use core::ffi::c_void;

#[repr(C)]
pub struct tskTCB {
    pub pxTopOfStack: *mut portSTACK_TYPE,
    pub xGenericListItem: crate::list::xListItem,
    pub xEventListItem: crate::list::xListItem,
    pub uxPriority: unsigned_portBASE_TYPE,
    pub pxStack: *mut portSTACK_TYPE,
    pub pcTaskName: [i8; configMAX_TASK_NAME_LEN],
    pub pxEndOfStack: *mut portSTACK_TYPE,
    pub uxCriticalNesting: unsigned_portBASE_TYPE,
    pub uxTCBNumber: unsigned_portBASE_TYPE,
    pub uxTaskNumber: unsigned_portBASE_TYPE,
    pub uxBasePriority: unsigned_portBASE_TYPE,
    pub pxTaskTag: pdTASK_HOOK_CODE,
    pub ulRunTimeCounter: u32,
}

#[repr(C)]
pub struct xMemoryRegion {
    pub pvBaseAddress: *mut c_void,
    pub ulLengthInBytes: unsigned_portBASE_TYPE,
    pub ulParameters: unsigned_portBASE_TYPE,
}

// Global variables
static mut pxCurrentTCB: *mut tskTCB = core::ptr::null_mut();
static mut uxCurrentNumberOfTasks: unsigned_portBASE_TYPE = 0;
static mut uxTaskNumber: unsigned_portBASE_TYPE = 0;
static mut xTasksWaitingTermination: xList = xList::new();
static mut uxTasksDeleted: unsigned_portBASE_TYPE = 0;

// Helper functions
pub fn prvGetTCBFromHandle(pxHandle: super::xTaskHandle) -> *mut tskTCB {
    if pxHandle.is_null() {
        unsafe { pxCurrentTCB }
    } else {
        pxHandle
    }
}

pub fn prvAllocateTCBAndStack(
    usStackDepth: u16,
    puxStackBuffer: *mut portSTACK_TYPE,
) -> *mut tskTCB {
    let pxNewTCB = unsafe {
        if puxStackBuffer.is_null() {
            // Allocate memory for the TCB and stack
            let total_size = core::mem::size_of::<tskTCB>()
                + (usStackDepth as usize * core::mem::size_of::<portSTACK_TYPE>());
            let pvNewTCB = crate::portable::pvPortMalloc(total_size);

            if !pvNewTCB.is_null() {
                let pxTCB = pvNewTCB as *mut tskTCB;
                // Set the stack pointer to the end of the TCB
                (*pxTCB).pxStack =
                    pvNewTCB.add(core::mem::size_of::<tskTCB>()) as *mut portSTACK_TYPE;
                pxTCB
            } else {
                core::ptr::null_mut()
            }
        } else {
            // Use the provided stack buffer
            let pvNewTCB = crate::portable::pvPortMalloc(core::mem::size_of::<tskTCB>());

            if !pvNewTCB.is_null() {
                let pxTCB = pvNewTCB as *mut tskTCB;
                (*pxTCB).pxStack = puxStackBuffer;
                pxTCB
            } else {
                core::ptr::null_mut()
            }
        }
    };

    if !pxNewTCB.is_null() {
        unsafe {
            // Initialize the TCB memory to zero
            core::ptr::write_bytes(pxNewTCB, 0, 1);
        }
    }

    pxNewTCB
}

pub fn prvDeleteTCB(pxTCB: *mut tskTCB) {
    unsafe {
        // Free the stack if it was allocated
        if !(*pxTCB).pxStack.is_null() {
            crate::portable::vPortFree((*pxTCB).pxStack as *mut c_void);
        }

        // Free the TCB
        crate::portable::vPortFree(pxTCB as *mut c_void);
    }
}

pub fn prvInitialiseTCBVariables(
    pxTCB: *mut tskTCB,
    pcName: *const i8,
    uxPriority: unsigned_portBASE_TYPE,
    xRegions: *const xMemoryRegion,
    usStackDepth: u16,
) {
    unsafe {
        // Initialize the task name
        if !pcName.is_null() {
            let mut i = 0;
            while i < configMAX_TASK_NAME_LEN {
                (*pxTCB).pcTaskName[i] = *pcName.add(i);
                if (*pxTCB).pcTaskName[i] == 0 {
                    break;
                }
                i += 1;
            }
            if i == configMAX_TASK_NAME_LEN {
                (*pxTCB).pcTaskName[configMAX_TASK_NAME_LEN - 1] = 0;
            }
        }

        // Initialize other TCB variables
        (*pxTCB).uxPriority = uxPriority;
        (*pxTCB).uxBasePriority = uxPriority;
        (*pxTCB).uxCriticalNesting = 0;
        (*pxTCB).pxTaskTag = None;
        (*pxTCB).ulRunTimeCounter = 0;

        // Set the stack end pointer
        (*pxTCB).pxEndOfStack = (*pxTCB).pxStack.add(usStackDepth as usize);

        // Initialize the task number
        (*pxTCB).uxTaskNumber = uxTaskNumber;
        uxTaskNumber += 1;

        // Initialize the TCB number
        (*pxTCB).uxTCBNumber = uxCurrentNumberOfTasks;
        uxCurrentNumberOfTasks += 1;
    }
}

// Task creation and deletion
pub fn xTaskCreate(
    pvTaskCode: unsafe extern "C" fn(*mut c_void),
    pcName: *const i8,
    usStackDepth: u16,
    pvParameters: *mut c_void,
    uxPriority: unsigned_portBASE_TYPE,
    pxCreatedTask: *mut super::xTaskHandle,
) -> portBASE_TYPE {
    xTaskGenericCreate(
        Some(pvTaskCode),
        pcName,
        usStackDepth,
        pvParameters,
        uxPriority,
        pxCreatedTask,
        core::ptr::null_mut(),
        core::ptr::null(),
    )
}

pub fn xTaskGenericCreate(
    pxTaskCode: super::pdTASK_CODE,
    pcName: *const i8,
    usStackDepth: u16,
    pvParameters: *mut c_void,
    uxPriority: unsigned_portBASE_TYPE,
    pxCreatedTask: *mut super::xTaskHandle,
    puxStackBuffer: *mut crate::portable::portSTACK_TYPE,
    xRegions: *const xMemoryRegion,
) -> portBASE_TYPE {
    unsafe {
        // Allocate TCB and stack
        let pxNewTCB = prvAllocateTCBAndStack(usStackDepth, puxStackBuffer);
        if pxNewTCB.is_null() {
            return super::pdFALSE;
        }

        // Initialize TCB variables
        prvInitialiseTCBVariables(pxNewTCB, pcName, uxPriority, xRegions, usStackDepth);

        // Initialize stack
        let pxTopOfStack =
            crate::portable::pxPortInitialiseStack(pxNewTCB, pxTaskCode, pvParameters);
        (*pxNewTCB).pxTopOfStack = pxTopOfStack;

        // Add task to ready list
        (*pxNewTCB).xGenericListItem.xItemValue = uxPriority;
        crate::list::vListInsert(
            &mut scheduler::xReadyTasksLists[uxPriority as usize],
            &mut (*pxNewTCB).xGenericListItem,
        );

        // Return task handle if requested
        if !pxCreatedTask.is_null() {
            *pxCreatedTask = pxNewTCB;
        }

        super::pdTRUE
    }
}

pub fn vTaskDelete(xTaskToDelete: super::xTaskHandle) {
    unsafe {
        let pxTCB = prvGetTCBFromHandle(xTaskToDelete);

        // Remove task from all lists
        if (*pxTCB).xGenericListItem.pxContainer != core::ptr::null_mut() {
            crate::list::vListRemove(&mut (*pxTCB).xGenericListItem);
        }
        if (*pxTCB).xEventListItem.pxContainer != core::ptr::null_mut() {
            crate::list::vListRemove(&mut (*pxTCB).xEventListItem);
        }

        // Add task to termination list
        crate::list::vListInsert(
            &mut scheduler::xTasksWaitingTermination,
            &mut (*pxTCB).xGenericListItem,
        );

        // Force context switch if needed
        if pxTCB == prvGetTCBFromHandle(core::ptr::null_mut()) {
            crate::portable::portYIELD_WITHIN_API();
        }
    }
}

// Task information
pub fn pcTaskGetTaskName(xTaskToQuery: super::xTaskHandle) -> *mut i8 {
    unsafe {
        let pxTCB = prvGetTCBFromHandle(xTaskToQuery);
        if pxTCB != core::ptr::null_mut() {
            (*pxTCB).pcTaskName.as_mut_ptr()
        } else {
            core::ptr::null_mut()
        }
    }
}

pub fn uxTaskGetTaskNumber(xTask: super::xTaskHandle) -> unsigned_portBASE_TYPE {
    unsafe {
        let pxTCB = prvGetTCBFromHandle(xTask);
        if pxTCB != core::ptr::null_mut() {
            (*pxTCB).uxTaskNumber
        } else {
            0
        }
    }
}

pub fn vTaskSetTaskNumber(xTask: super::xTaskHandle, uxHandle: unsigned_portBASE_TYPE) {
    unsafe {
        let pxTCB = prvGetTCBFromHandle(xTask);
        if pxTCB != core::ptr::null_mut() {
            (*pxTCB).uxTaskNumber = uxHandle;
        }
    }
}

// Task hook functions
pub fn vTaskSetApplicationTaskTag(
    xTask: super::xTaskHandle,
    pxHookFunction: super::pdTASK_HOOK_CODE,
) {
    unsafe {
        let pxTCB = prvGetTCBFromHandle(xTask);
        if pxTCB != core::ptr::null_mut() {
            (*pxTCB).pxTaskTag = pxHookFunction;
        }
    }
}

pub fn xTaskGetApplicationTaskTag(xTask: super::xTaskHandle) -> super::pdTASK_HOOK_CODE {
    unsafe {
        let pxTCB = prvGetTCBFromHandle(xTask);
        if pxTCB != core::ptr::null_mut() {
            (*pxTCB).pxTaskTag
        } else {
            None
        }
    }
}

pub fn xTaskCallApplicationTaskHook(
    xTask: super::xTaskHandle,
    pvParameter: *mut c_void,
) -> portBASE_TYPE {
    unsafe {
        let pxTCB = prvGetTCBFromHandle(xTask);
        if pxTCB != core::ptr::null_mut() {
            if let Some(pxHookFunction) = (*pxTCB).pxTaskTag {
                return pxHookFunction(pvParameter);
            }
        }
        super::pdFALSE
    }
}

pub fn prvAddTaskToReadyQueue(pxTCB: *mut tskTCB) {
    unsafe {
        if (*pxTCB).uxPriority > scheduler::uxTopReadyPriority {
            scheduler::uxTopReadyPriority = (*pxTCB).uxPriority;
        }
        crate::list::vListInsertEnd(
            &mut scheduler::xReadyTasksLists[(*pxTCB).uxPriority as usize],
            &mut (*pxTCB).xGenericListItem,
        );
    }
}

pub fn prvCheckTasksWaitingTermination() {
    unsafe {
        while !crate::list::listLIST_IS_EMPTY(&xTasksWaitingTermination) {
            let pxTCB =
                crate::list::listGET_OWNER_OF_HEAD_ENTRY(&xTasksWaitingTermination) as *mut tskTCB;

            // Remove the task from the termination list
            crate::list::vListRemove(&mut (*pxTCB).xGenericListItem);

            // Decrement the task counter
            uxCurrentNumberOfTasks -= 1;

            // Delete the TCB
            prvDeleteTCB(pxTCB);
        }
    }
}
