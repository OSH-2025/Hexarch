#![no_std]
use super::tcb::tskTCB;
use crate::portable::{portBASE_TYPE, unsigned_portBASE_TYPE};
use core::ffi::c_void;
use core::sync::atomic::{AtomicU32, Ordering};

// Constants
const configTASK_NOTIFICATION_ARRAY_ENTRIES: usize = 1;
const taskNOTIFICATION_RECEIVED_BIT: u32 = 0x80000000;
const taskNOTIFICATION_WAITING_BIT: u32 = 0x40000000;
const taskNOTIFICATION_VALUE_MASK: u32 = 0x00FFFFFF;

// Task notification structure
#[repr(C)]
pub struct TaskNotification {
    pub ulNotifiedValue: AtomicU32,
    pub ucNotifyState: u8,
}

impl TaskNotification {
    pub const fn new() -> Self {
        Self {
            ulNotifiedValue: AtomicU32::new(0),
            ucNotifyState: 0,
        }
    }
}

// Task notification action enum
#[repr(C)]
pub enum eNotifyAction {
    eNoAction = 0,
    eSetBits = 1,
    eIncrement = 2,
    eSetValueWithOverwrite = 3,
    eSetValueWithoutOverwrite = 4,
}

// Global variables
static mut xTaskNotificationArray: [TaskNotification; configTASK_NOTIFICATION_ARRAY_ENTRIES] =
    [TaskNotification::new(); configTASK_NOTIFICATION_ARRAY_ENTRIES];

// Task notification functions
pub fn xTaskNotify(
    xTaskToNotify: super::xTaskHandle,
    ulValue: u32,
    eAction: eNotifyAction,
) -> portBASE_TYPE {
    unsafe {
        let pxTCB = super::tcb::prvGetTCBFromHandle(xTaskToNotify);
        if pxTCB == core::ptr::null_mut() {
            return super::pdFALSE;
        }

        let pxNotification = &mut xTaskNotificationArray[(*pxTCB).uxTCBNumber as usize];
        let ulCurrentValue = pxNotification.ulNotifiedValue.load(Ordering::Relaxed);

        match eAction {
            eNotifyAction::eNoAction => {
                // No action
                pxNotification
                    .ulNotifiedValue
                    .store(ulValue, Ordering::Relaxed);
            }
            eNotifyAction::eSetBits => {
                // Set bits
                pxNotification
                    .ulNotifiedValue
                    .store(ulCurrentValue | ulValue, Ordering::Relaxed);
            }
            eNotifyAction::eIncrement => {
                // Increment
                pxNotification
                    .ulNotifiedValue
                    .store(ulCurrentValue + 1, Ordering::Relaxed);
            }
            eNotifyAction::eSetValueWithOverwrite => {
                // Set value with overwrite
                pxNotification
                    .ulNotifiedValue
                    .store(ulValue, Ordering::Relaxed);
            }
            eNotifyAction::eSetValueWithoutOverwrite => {
                // Set value without overwrite
                if (ulCurrentValue & taskNOTIFICATION_RECEIVED_BIT) == 0 {
                    pxNotification
                        .ulNotifiedValue
                        .store(ulValue, Ordering::Relaxed);
                }
            }
        }

        // Set the notification state
        pxNotification.ucNotifyState = 1;

        // If the task is waiting for a notification, remove it from the event list
        if (ulCurrentValue & taskNOTIFICATION_WAITING_BIT) != 0 {
            crate::list::vListRemove(&mut (*pxTCB).xEventListItem);
            super::tcb::prvAddTaskToReadyQueue(pxTCB);
        }

        super::pdTRUE
    }
}

pub fn xTaskNotifyWait(
    ulBitsToClearOnEntry: u32,
    ulBitsToClearOnExit: u32,
    pulNotificationValue: *mut u32,
    xTicksToWait: crate::portable::portTickType,
) -> portBASE_TYPE {
    unsafe {
        let pxTCB = super::tcb::prvGetTCBFromHandle(core::ptr::null_mut());
        let pxNotification = &mut xTaskNotificationArray[(*pxTCB).uxTCBNumber as usize];
        let ulCurrentValue = pxNotification.ulNotifiedValue.load(Ordering::Relaxed);

        // Clear the bits on entry
        pxNotification
            .ulNotifiedValue
            .store(ulCurrentValue & !ulBitsToClearOnEntry, Ordering::Relaxed);

        // If a notification is already pending, return immediately
        if pxNotification.ucNotifyState != 0 {
            pxNotification.ucNotifyState = 0;
            if !pulNotificationValue.is_null() {
                *pulNotificationValue = pxNotification.ulNotifiedValue.load(Ordering::Relaxed)
                    & taskNOTIFICATION_VALUE_MASK;
            }
            return super::pdTRUE;
        }

        // Set the waiting bit
        pxNotification.ulNotifiedValue.store(
            ulCurrentValue | taskNOTIFICATION_WAITING_BIT,
            Ordering::Relaxed,
        );

        // Add the task to the event list
        super::scheduler::vTaskPlaceOnEventList(
            &mut super::scheduler::xTasksWaitingNotification,
            xTicksToWait,
        );

        // Clear the bits on exit
        pxNotification.ulNotifiedValue.store(
            pxNotification.ulNotifiedValue.load(Ordering::Relaxed) & !ulBitsToClearOnExit,
            Ordering::Relaxed,
        );

        // Return the notification value
        if !pulNotificationValue.is_null() {
            *pulNotificationValue = pxNotification.ulNotifiedValue.load(Ordering::Relaxed)
                & taskNOTIFICATION_VALUE_MASK;
        }

        super::pdTRUE
    }
}

extern "C" {
    fn portYIELD_WITHIN_API();
}

// External variables
extern "C" {
    static mut xReadyTasksLists: [crate::list::xList; 32];
    static mut xDelayedTaskList: crate::list::xList;
}
