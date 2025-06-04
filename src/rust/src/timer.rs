#![no_std]

// Timer configuration constants
pub const configTIMER_TASK_STACK_DEPTH: usize = 256;
pub const configTIMER_TASK_PRIORITY: u8 = 2;
pub const tmrNO_DELAY: portTickType = 0;

use crate::list::{xList, xListItem};
use crate::task::{
    pdFAIL, pdFALSE, pdPASS, pdTRUE, portBASE_TYPE, portPRIVILEGE_BIT, portTickType,
    taskSCHEDULER_RUNNING,
};
use core::ffi::c_void;

pub type xTimerHandle = *mut xTIMER;
pub type tmrTIMER_CALLBACK = Option<unsafe extern "C" fn(xTimerHandle)>;

#[repr(C)]
pub struct xTIMER {
    pub pcTimerName: *const i8,
    pub xTimerListItem: xListItem,
    pub xTimerPeriodInTicks: portTickType,
    pub uxAutoReload: portBASE_TYPE,
    pub pvTimerID: *mut c_void,
    pub pxCallbackFunction: tmrTIMER_CALLBACK,
}

#[repr(C)]
pub struct xTIMER_MESSAGE {
    pub xMessageID: portBASE_TYPE,
    pub xMessageValue: portTickType,
    pub pxTimer: *mut xTIMER,
}

// Timer command IDs
pub const tmrCOMMAND_START: portBASE_TYPE = 0;
pub const tmrCOMMAND_STOP: portBASE_TYPE = 1;
pub const tmrCOMMAND_CHANGE_PERIOD: portBASE_TYPE = 2;
pub const tmrCOMMAND_DELETE: portBASE_TYPE = 3;
pub const tmrCOMMAND_RESET: portBASE_TYPE = 4;
pub const tmrCOMMAND_EXECUTE_CALLBACK: portBASE_TYPE = 5;

// Global variables
static mut xActiveTimerList1: xList = unsafe { core::mem::zeroed() };
static mut xActiveTimerList2: xList = unsafe { core::mem::zeroed() };
static mut pxCurrentTimerList: *mut xList = unsafe { core::ptr::null_mut() };
static mut pxOverflowTimerList: *mut xList = unsafe { core::ptr::null_mut() };
static mut xTimerQueue: *mut c_void = core::ptr::null_mut();
static mut xTimerTaskHandle: *mut c_void = core::ptr::null_mut();

// External function declarations
extern "C" {
    fn pvPortMalloc(xWantedSize: usize) -> *mut c_void;
    fn vPortFree(pv: *mut c_void);
    fn xQueueSendToBack(
        xQueue: *mut c_void,
        pvItemToQueue: *const c_void,
        xTicksToWait: portTickType,
    ) -> portBASE_TYPE;
    fn xQueueSendToBackFromISR(
        xQueue: *mut c_void,
        pvItemToQueue: *const c_void,
        pxHigherPriorityTaskWoken: *mut portBASE_TYPE,
    ) -> portBASE_TYPE;
    fn vQueueWaitForMessageRestricted(xQueue: *mut c_void, xTicksToWait: portTickType);
    fn xTaskGetSchedulerState() -> portBASE_TYPE;
    fn vTaskSuspendAll();
    fn xTaskResumeAll() -> portBASE_TYPE;
    fn portYIELD_WITHIN_API();
}

// Function prototypes
fn prvCheckForValidListAndQueue();
fn prvTimerTask(pvParameters: *mut c_void);
fn prvProcessReceivedCommands();
fn prvInsertTimerInActiveList(
    pxTimer: *mut xTIMER,
    xNextExpiryTime: portTickType,
    xTimeNow: portTickType,
    xCommandTime: portTickType,
) -> portBASE_TYPE;
fn prvProcessExpiredTimer(xNextExpireTime: portTickType, xTimeNow: portTickType);
fn prvSwitchTimerLists(xLastTime: portTickType);
fn prvSampleTimeNow(pxTimerListsWereSwitched: *mut portBASE_TYPE) -> portTickType;
fn prvGetNextExpireTime(pxListWasEmpty: *mut portBASE_TYPE) -> portTickType;
fn prvProcessTimerOrBlockTask(xNextExpireTime: portTickType, xListWasEmpty: portBASE_TYPE);

fn prvCheckForValidListAndQueue() {
    unsafe {
        // Check if the timer lists are initialized
        if pxCurrentTimerList.is_null() {
            // Initialize the timer lists
            pxCurrentTimerList = &mut xActiveTimerList1 as *mut _;
            pxOverflowTimerList = &mut xActiveTimerList2 as *mut _;

            // Initialize the lists
            crate::list::vListInitialise(pxCurrentTimerList);
            crate::list::vListInitialise(pxOverflowTimerList);
        }
    }
}

fn prvTimerTask(pvParameters: *mut c_void) {
    let mut xNextExpireTime: portTickType;
    let mut xListWasEmpty: portBASE_TYPE;

    // Remove compiler warning about unused parameter
    let _ = pvParameters;

    // Initialize the timer lists
    prvCheckForValidListAndQueue();

    // Main timer task loop
    loop {
        // Get the next expire time
        xNextExpireTime = prvGetNextExpireTime(&mut xListWasEmpty);

        // Process timers or block until next timer expires
        prvProcessTimerOrBlockTask(xNextExpireTime, xListWasEmpty);

        // Process any received commands
        prvProcessReceivedCommands();
    }
}

fn prvProcessReceivedCommands() {
    let mut xMessage: xTIMER_MESSAGE;
    let mut xTimerListsWereSwitched: portBASE_TYPE;
    let mut xResult: portBASE_TYPE;
    let mut xTimeNow: portTickType;

    unsafe {
        // Process all pending timer commands
        while xQueueSendToBack(xTimerQueue, &xMessage as *const _ as *const c_void, 0) == pdTRUE {
            if xMessage.xMessageID == tmrCOMMAND_EXECUTE_CALLBACK {
                // Execute the callback function
                if let Some(callback) = (*xMessage.pxTimer).pxCallbackFunction {
                    callback(xMessage.pxTimer);
                }

                // If the timer is auto-reload, reinsert it into the active list
                if (*xMessage.pxTimer).uxAutoReload != 0 {
                    xTimeNow = prvSampleTimeNow(&mut xTimerListsWereSwitched);
                    prvInsertTimerInActiveList(
                        xMessage.pxTimer,
                        xTimeNow + (*xMessage.pxTimer).xTimerPeriodInTicks,
                        xTimeNow,
                        xTimeNow,
                    );
                }
            } else if xMessage.xMessageID == tmrCOMMAND_START {
                // Start the timer
                xTimeNow = prvSampleTimeNow(&mut xTimerListsWereSwitched);
                xResult = prvInsertTimerInActiveList(
                    xMessage.pxTimer,
                    xTimeNow + (*xMessage.pxTimer).xTimerPeriodInTicks,
                    xTimeNow,
                    xTimeNow,
                );

                if xResult != pdPASS {
                    // Timer could not be started
                    vPortFree(xMessage.pxTimer as *mut c_void);
                }
            } else if xMessage.xMessageID == tmrCOMMAND_STOP {
                // Stop the timer
                crate::list::vListRemove(&mut (*xMessage.pxTimer).xTimerListItem as *mut _);
            } else if xMessage.xMessageID == tmrCOMMAND_CHANGE_PERIOD {
                // Change the timer period
                xTimeNow = prvSampleTimeNow(&mut xTimerListsWereSwitched);
                (*xMessage.pxTimer).xTimerPeriodInTicks = xMessage.xMessageValue;
                prvInsertTimerInActiveList(
                    xMessage.pxTimer,
                    xTimeNow + (*xMessage.pxTimer).xTimerPeriodInTicks,
                    xTimeNow,
                    xTimeNow,
                );
            } else if xMessage.xMessageID == tmrCOMMAND_DELETE {
                // Delete the timer
                vPortFree(xMessage.pxTimer as *mut c_void);
            } else if xMessage.xMessageID == tmrCOMMAND_RESET {
                // Reset the timer
                xTimeNow = prvSampleTimeNow(&mut xTimerListsWereSwitched);
                prvInsertTimerInActiveList(
                    xMessage.pxTimer,
                    xTimeNow + (*xMessage.pxTimer).xTimerPeriodInTicks,
                    xTimeNow,
                    xTimeNow,
                );
            }
        }
    }
}

fn prvInsertTimerInActiveList(
    pxTimer: *mut xTIMER,
    xNextExpiryTime: portTickType,
    xTimeNow: portTickType,
    xCommandTime: portTickType,
) -> portBASE_TYPE {
    let mut xProcessTimerNow: portBASE_TYPE = pdFALSE;

    unsafe {
        // Set the timer's expiry time
        (*pxTimer).xTimerListItem.xItemValue = xNextExpiryTime;

        // Check if the timer should be processed now
        if xNextExpiryTime <= xTimeNow {
            if xTimeNow - xCommandTime >= (*pxTimer).xTimerPeriodInTicks {
                xProcessTimerNow = pdTRUE;
            } else {
                // The timer has expired, but the command was received after the timer
                // should have expired, so the timer is inserted into the current list
                crate::list::vListInsert(
                    pxCurrentTimerList,
                    &mut (*pxTimer).xTimerListItem as *mut _,
                );
            }
        } else {
            // The timer has not expired, so it is inserted into the overflow list
            if xNextExpiryTime < xCommandTime {
                // The timer has expired, but the command was received before the timer
                // should have expired, so the timer is inserted into the current list
                crate::list::vListInsert(
                    pxCurrentTimerList,
                    &mut (*pxTimer).xTimerListItem as *mut _,
                );
            } else {
                // The timer has not expired, so it is inserted into the overflow list
                crate::list::vListInsert(
                    pxOverflowTimerList,
                    &mut (*pxTimer).xTimerListItem as *mut _,
                );
            }
        }
    }

    xProcessTimerNow
}

fn prvProcessExpiredTimer(xNextExpireTime: portTickType, xTimeNow: portTickType) {
    let mut xMessage: xTIMER_MESSAGE;
    let mut pxTimer: *mut xTIMER;

    unsafe {
        // Process all expired timers
        while !crate::list::listLIST_IS_EMPTY(pxCurrentTimerList) {
            // Get the first timer in the list
            pxTimer = crate::list::listGET_OWNER_OF_HEAD_ENTRY(pxCurrentTimerList) as *mut xTIMER;

            // Remove the timer from the list
            crate::list::vListRemove(&mut (*pxTimer).xTimerListItem as *mut _);

            // Send a message to execute the timer's callback function
            xMessage.xMessageID = tmrCOMMAND_EXECUTE_CALLBACK;
            xMessage.pxTimer = pxTimer;
            xMessage.xMessageValue = 0;

            xQueueSendToBack(xTimerQueue, &xMessage as *const _ as *const c_void, 0);
        }
    }
}

fn prvSwitchTimerLists(xLastTime: portTickType) {
    let mut pxTemp: *mut xList;

    unsafe {
        // Swap the timer lists
        pxTemp = pxCurrentTimerList;
        pxCurrentTimerList = pxOverflowTimerList;
        pxOverflowTimerList = pxTemp;

        // Process any timers that have expired
        prvProcessExpiredTimer(xLastTime, xLastTime);
    }
}

fn prvSampleTimeNow(pxTimerListsWereSwitched: *mut portBASE_TYPE) -> portTickType {
    let mut xTimeNow: portTickType;
    let mut xLastTime: portTickType;

    unsafe {
        // Get the current tick count
        xTimeNow = crate::task::xTaskGetTickCount();

        // Check if the timer lists need to be switched
        xLastTime = (*pxCurrentTimerList).xListEnd.xItemValue;
        if xTimeNow < xLastTime {
            // The timer lists need to be switched
            prvSwitchTimerLists(xLastTime);
            *pxTimerListsWereSwitched = pdTRUE;
        } else {
            *pxTimerListsWereSwitched = pdFALSE;
        }
    }

    xTimeNow
}

fn prvGetNextExpireTime(pxListWasEmpty: *mut portBASE_TYPE) -> portTickType {
    let mut xNextExpireTime: portTickType;

    unsafe {
        // Check if the current timer list is empty
        if crate::list::listLIST_IS_EMPTY(pxCurrentTimerList) {
            // The list is empty
            *pxListWasEmpty = pdTRUE;
            xNextExpireTime = 0;
        } else {
            // The list is not empty
            *pxListWasEmpty = pdFALSE;
            xNextExpireTime = (*pxCurrentTimerList).xListEnd.xItemValue;
        }
    }

    xNextExpireTime
}

fn prvProcessTimerOrBlockTask(xNextExpireTime: portTickType, xListWasEmpty: portBASE_TYPE) {
    let mut xTimeNow: portTickType;
    let mut xTimerListsWereSwitched: portBASE_TYPE;

    unsafe {
        // Get the current time
        xTimeNow = prvSampleTimeNow(&mut xTimerListsWereSwitched);

        // Check if there are any timers to process
        if xListWasEmpty == pdFALSE {
            // There are timers to process
            if xNextExpireTime <= xTimeNow {
                // The next timer has expired
                prvProcessExpiredTimer(xNextExpireTime, xTimeNow);
            } else {
                // The next timer has not expired, so block until it does
                vQueueWaitForMessageRestricted(xTimerQueue, xNextExpireTime - xTimeNow);
            }
        } else {
            // There are no timers to process, so block indefinitely
            vQueueWaitForMessageRestricted(xTimerQueue, portMAX_DELAY);
        }
    }
}

#[no_mangle]
pub extern "C" fn xTimerCreate(
    pcTimerName: *const i8,
    xTimerPeriodInTicks: portTickType,
    uxAutoReload: portBASE_TYPE,
    pvTimerID: *mut c_void,
    pxCallbackFunction: tmrTIMER_CALLBACK,
) -> xTimerHandle {
    let pxNewTimer: *mut xTIMER;

    unsafe {
        // Allocate memory for the timer
        pxNewTimer = pvPortMalloc(core::mem::size_of::<xTIMER>()) as *mut xTIMER;

        if pxNewTimer.is_null() {
            // Memory allocation failed
            return core::ptr::null_mut();
        }

        // Initialize the timer
        (*pxNewTimer).pcTimerName = pcTimerName;
        (*pxNewTimer).xTimerPeriodInTicks = xTimerPeriodInTicks;
        (*pxNewTimer).uxAutoReload = uxAutoReload;
        (*pxNewTimer).pvTimerID = pvTimerID;
        (*pxNewTimer).pxCallbackFunction = pxCallbackFunction;

        // Initialize the timer's list item
        crate::list::vListInitialiseItem(&mut (*pxNewTimer).xTimerListItem);
        (*pxNewTimer).xTimerListItem.pvOwner = pxNewTimer as *mut c_void;
    }

    pxNewTimer
}

#[no_mangle]
pub extern "C" fn vTimerSetTimerID(pxTimer: xTimerHandle, pvNewID: *mut c_void) {
    unsafe {
        (*pxTimer).pvTimerID = pvNewID;
    }
}

#[no_mangle]
pub extern "C" fn pvTimerGetTimerID(pxTimer: xTimerHandle) -> *mut c_void {
    unsafe { (*pxTimer).pvTimerID }
}

#[no_mangle]
pub extern "C" fn xTimerIsTimerActive(pxTimer: xTimerHandle) -> portBASE_TYPE {
    let xReturn: portBASE_TYPE;

    unsafe {
        // Check if the timer is in a list
        if (*pxTimer).xTimerListItem.pvContainer.is_null() {
            xReturn = pdFALSE;
        } else {
            xReturn = pdTRUE;
        }
    }

    xReturn
}

#[no_mangle]
pub extern "C" fn xTimerStart(pxTimer: xTimerHandle, xTicksToWait: portTickType) -> portBASE_TYPE {
    let xMessage: xTIMER_MESSAGE;
    let xReturn: portBASE_TYPE;

    unsafe {
        // Send a message to start the timer
        xMessage.xMessageID = tmrCOMMAND_START;
        xMessage.pxTimer = pxTimer;
        xMessage.xMessageValue = 0;

        xReturn = xQueueSendToBack(
            xTimerQueue,
            &xMessage as *const _ as *const c_void,
            xTicksToWait,
        );
    }

    xReturn
}

#[no_mangle]
pub extern "C" fn xTimerStop(pxTimer: xTimerHandle, xTicksToWait: portTickType) -> portBASE_TYPE {
    let xMessage: xTIMER_MESSAGE;
    let xReturn: portBASE_TYPE;

    unsafe {
        // Send a message to stop the timer
        xMessage.xMessageID = tmrCOMMAND_STOP;
        xMessage.pxTimer = pxTimer;
        xMessage.xMessageValue = 0;

        xReturn = xQueueSendToBack(
            xTimerQueue,
            &xMessage as *const _ as *const c_void,
            xTicksToWait,
        );
    }

    xReturn
}

#[no_mangle]
pub extern "C" fn xTimerChangePeriod(
    pxTimer: xTimerHandle,
    xNewPeriod: portTickType,
    xTicksToWait: portTickType,
) -> portBASE_TYPE {
    let xMessage: xTIMER_MESSAGE;
    let xReturn: portBASE_TYPE;

    unsafe {
        // Send a message to change the timer period
        xMessage.xMessageID = tmrCOMMAND_CHANGE_PERIOD;
        xMessage.pxTimer = pxTimer;
        xMessage.xMessageValue = xNewPeriod;

        xReturn = xQueueSendToBack(
            xTimerQueue,
            &xMessage as *const _ as *const c_void,
            xTicksToWait,
        );
    }

    xReturn
}

#[no_mangle]
pub extern "C" fn xTimerDelete(pxTimer: xTimerHandle, xTicksToWait: portTickType) -> portBASE_TYPE {
    let xMessage: xTIMER_MESSAGE;
    let xReturn: portBASE_TYPE;

    unsafe {
        // Send a message to delete the timer
        xMessage.xMessageID = tmrCOMMAND_DELETE;
        xMessage.pxTimer = pxTimer;
        xMessage.xMessageValue = 0;

        xReturn = xQueueSendToBack(
            xTimerQueue,
            &xMessage as *const _ as *const c_void,
            xTicksToWait,
        );
    }

    xReturn
}

#[no_mangle]
pub extern "C" fn xTimerReset(pxTimer: xTimerHandle, xTicksToWait: portTickType) -> portBASE_TYPE {
    let xMessage: xTIMER_MESSAGE;
    let xReturn: portBASE_TYPE;

    unsafe {
        // Send a message to reset the timer
        xMessage.xMessageID = tmrCOMMAND_RESET;
        xMessage.pxTimer = pxTimer;
        xMessage.xMessageValue = 0;

        xReturn = xQueueSendToBack(
            xTimerQueue,
            &xMessage as *const _ as *const c_void,
            xTicksToWait,
        );
    }

    xReturn
}

#[no_mangle]
pub extern "C" fn xTimerGetPeriod(pxTimer: xTimerHandle) -> portTickType {
    unsafe { (*pxTimer).xTimerPeriodInTicks }
}

#[no_mangle]
pub extern "C" fn xTimerStartFromISR(
    pxTimer: xTimerHandle,
    pxHigherPriorityTaskWoken: *mut portBASE_TYPE,
) -> portBASE_TYPE {
    let xMessage: xTIMER_MESSAGE;
    let xReturn: portBASE_TYPE;

    unsafe {
        // Send a message to start the timer
        xMessage.xMessageID = tmrCOMMAND_START;
        xMessage.pxTimer = pxTimer;
        xMessage.xMessageValue = 0;

        xReturn = xQueueSendToBackFromISR(
            xTimerQueue,
            &xMessage as *const _ as *const c_void,
            pxHigherPriorityTaskWoken,
        );
    }

    xReturn
}

#[no_mangle]
pub extern "C" fn xTimerStopFromISR(
    pxTimer: xTimerHandle,
    pxHigherPriorityTaskWoken: *mut portBASE_TYPE,
) -> portBASE_TYPE {
    let xMessage: xTIMER_MESSAGE;
    let xReturn: portBASE_TYPE;

    unsafe {
        // Send a message to stop the timer
        xMessage.xMessageID = tmrCOMMAND_STOP;
        xMessage.pxTimer = pxTimer;
        xMessage.xMessageValue = 0;

        xReturn = xQueueSendToBackFromISR(
            xTimerQueue,
            &xMessage as *const _ as *const c_void,
            pxHigherPriorityTaskWoken,
        );
    }

    xReturn
}

#[no_mangle]
pub extern "C" fn xTimerChangePeriodFromISR(
    pxTimer: xTimerHandle,
    xNewPeriod: portTickType,
    pxHigherPriorityTaskWoken: *mut portBASE_TYPE,
) -> portBASE_TYPE {
    let xMessage: xTIMER_MESSAGE;
    let xReturn: portBASE_TYPE;

    unsafe {
        // Send a message to change the timer period
        xMessage.xMessageID = tmrCOMMAND_CHANGE_PERIOD;
        xMessage.pxTimer = pxTimer;
        xMessage.xMessageValue = xNewPeriod;

        xReturn = xQueueSendToBackFromISR(
            xTimerQueue,
            &xMessage as *const _ as *const c_void,
            pxHigherPriorityTaskWoken,
        );
    }

    xReturn
}

#[no_mangle]
pub extern "C" fn xTimerResetFromISR(
    pxTimer: xTimerHandle,
    pxHigherPriorityTaskWoken: *mut portBASE_TYPE,
) -> portBASE_TYPE {
    let xMessage: xTIMER_MESSAGE;
    let xReturn: portBASE_TYPE;

    unsafe {
        // Send a message to reset the timer
        xMessage.xMessageID = tmrCOMMAND_RESET;
        xMessage.pxTimer = pxTimer;
        xMessage.xMessageValue = 0;

        xReturn = xQueueSendToBackFromISR(
            xTimerQueue,
            &xMessage as *const _ as *const c_void,
            pxHigherPriorityTaskWoken,
        );
    }

    xReturn
}

#[no_mangle]
pub extern "C" fn xTimerPendFunctionCallFromISR(
    pvFunctionToPend: *mut c_void,
    pvParameter1: *mut c_void,
    ulParameter2: usize,
    pxHigherPriorityTaskWoken: *mut portBASE_TYPE,
) -> portBASE_TYPE {
    let xMessage: xTIMER_MESSAGE;
    let xReturn: portBASE_TYPE;

    unsafe {
        // Send a message to execute the function
        xMessage.xMessageID = tmrCOMMAND_EXECUTE_CALLBACK;
        xMessage.pxTimer = pvFunctionToPend as *mut xTIMER;
        xMessage.xMessageValue = ulParameter2 as portTickType;

        xReturn = xQueueSendToBackFromISR(
            xTimerQueue,
            &xMessage as *const _ as *const c_void,
            pxHigherPriorityTaskWoken,
        );
    }

    xReturn
}

#[no_mangle]
pub extern "C" fn xTimerCreateTimerTask() -> portBASE_TYPE {
    let mut xReturn: portBASE_TYPE = pdFAIL;

    unsafe {
        // Initialize the timer lists and queue
        prvCheckForValidListAndQueue();

        if !xTimerQueue.is_null() {
            // Create the timer task
            xReturn = xTaskCreate(
                Some(prvTimerTask),
                b"Tmr Svc\0" as *const u8 as *const i8,
                configTIMER_TASK_STACK_DEPTH as u16,
                core::ptr::null_mut(),
                (configTIMER_TASK_PRIORITY | portPRIVILEGE_BIT) as u8,
                &mut xTimerTaskHandle,
            );
        }
    }

    xReturn
}

#[no_mangle]
pub extern "C" fn xTimerGetTimerDaemonTaskHandle() -> *mut c_void {
    unsafe { xTimerTaskHandle }
}

#[no_mangle]
pub extern "C" fn xTimerGenericCommand(
    xTimer: xTimerHandle,
    xCommandID: portBASE_TYPE,
    xOptionalValue: portTickType,
    pxHigherPriorityTaskWoken: *mut portBASE_TYPE,
    xBlockTime: portTickType,
) -> portBASE_TYPE {
    let mut xReturn: portBASE_TYPE = pdFAIL;
    let mut xMessage: xTIMER_MESSAGE;

    unsafe {
        if !xTimerQueue.is_null() {
            // Prepare the message
            xMessage.xMessageID = xCommandID;
            xMessage.xMessageValue = xOptionalValue;
            xMessage.pxTimer = xTimer;

            if pxHigherPriorityTaskWoken.is_null() {
                // Not called from an ISR
                if xTaskGetSchedulerState() == taskSCHEDULER_RUNNING {
                    xReturn = xQueueSendToBack(
                        xTimerQueue,
                        &xMessage as *const _ as *const c_void,
                        xBlockTime,
                    );
                } else {
                    xReturn =
                        xQueueSendToBack(xTimerQueue, &xMessage as *const _ as *const c_void, 0);
                }
            } else {
                // Called from an ISR
                xReturn = xQueueSendToBackFromISR(
                    xTimerQueue,
                    &xMessage as *const _ as *const c_void,
                    pxHigherPriorityTaskWoken,
                );
            }
        }
    }

    xReturn
}
