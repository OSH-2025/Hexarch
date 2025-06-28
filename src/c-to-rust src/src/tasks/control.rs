//! Task control and management functions

#[macro_use]
use super::types::*;
#[macro_use]
use super::scheduler::*;
#[macro_use]
use super::creation::*;
extern crate alloc;
use crate::prvAddCurrentTaskToDelayedList;
use crate::prvResetNextTaskUnblockTime;
use super::scheduler::xTaskIncrementTick;
use crate::base::FreeRTOSconfig::{configMAX_PRIORITIES, CONFIG_ISR_STACK_SIZE_WORDS, USER_STACK_SIZE};
// use crate::base::FreeRTOSconfig::{configMAX_PRIORITIES, CONFIG_ISR_STACK_SIZE_WORDS, USER_STACK_SIZE};
use crate::base::kernel::{IDLE_p, IDLE_STACK};
// use crate::base::{IDLE_p, IDLE_STACK};
use crate::base::kernel::*;
// use crate::base::list::*;
use crate::list::*;
// use crate::list::*;
use crate::base::projdefs::*;
use crate::portable::portable::*;
use crate::portable::portmacro::*;
use crate::portable::riscv_virt::*;
#[cfg(feature = "configUSE_IDLE_HOOK")]
use crate::vApplicationIdleHook;
use crate::{
    mtCOVERAGE_TEST_MARKER, portDISABLE_INTERRUPTS, portENABLE_INTERRUPTS, portENTER_CRITICAL,
    portEXIT_CRITICAL, portYIELD, portYIELD_WITHIN_API,
};
use alloc::format;
use alloc::string::String;
use alloc::string::ToString;
use alloc::sync::{Arc, Weak};
use core::arch::asm;
use core::clone;
use core::cmp::max;
use core::ffi::c_void;
use core::mem::size_of;
use core::ptr::NonNull;
use spin::RwLock;

// 宏定义
#[macro_export]
macro_rules! taskENTER_CRITICAL {
    () => {
        portENTER_CRITICAL!();
    };
}

#[macro_export]
macro_rules! taskEXIT_CRITICAL {
    () => {
        portEXIT_CRITICAL!();
    };
}

#[macro_export]
macro_rules! vTaskMissedYield {
    () => {
        unsafe {
            xYieldPending = true;
        }
    };
}
#[macro_export]
macro_rules! taskYIELD_IF_USING_PREEMPTION {
    () => {
        portYIELD_WITHIN_API!();
    };
}

#[macro_export]
macro_rules! get_uxCurrentNumberOfTasks {
    () => {
        unsafe { crate::tasks::types::uxCurrentNumberOfTasks }
    };
}

#[macro_export]
macro_rules! get_scheduler_running {
    () => {
        unsafe { crate::xSchedulerRunning }
    };
}
/// Set priority of target task.
pub fn vTaskPrioritySet(pxTask: Option<&TaskHandle_t>, uxNewPriority: UBaseType) {
    // ... 实现代码
    vTaskEnterCritical();
    match pxTask {
        Some(x) => {
            uxListRemove(Arc::downgrade(&x.read().xStateListItem));
            vListInsertEnd(
                &READY_TASK_LISTS[uxNewPriority as usize],
                &x.read().xStateListItem,
            );
            x.write().uxPriority = uxNewPriority;
            listItemSetValue(
                &x.write().xEventListItem,
                configMAX_PRIORITIES - uxNewPriority,
            );
        }
        None => match get_current_tcb() {
            Some(x) => {
                uxListRemove(Arc::downgrade(&(*x).xStateListItem));
                vListInsertEnd(&READY_TASK_LISTS[uxNewPriority as usize], &x.xStateListItem);
                x.uxPriority = uxNewPriority;
                listItemSetValue(&x.xEventListItem, configMAX_PRIORITIES - uxNewPriority);
            }
            None => {}
        },
    }
    vTaskExitCritical();
}

/// Get priority of target task.
pub fn uxTaskPriorityGet(pxTask: Option<TaskHandle_t>) -> UBaseType {
    // ... 实现代码
    match get_current_tcb() {
        Some(x) => unsafe {
            return (*x).uxPriority;
        },
        None => {
            return 0;
        }
    }
}

/// Enter critical section
pub fn vTaskEnterCritical() {
    // ... 实现代码
    portDISABLE_INTERRUPTS!();
    unsafe {
        if XSCHEDULERRUNNING != pdFALSE {
            match get_current_tcb() {
                Some(x) => {
                    x.uxCriticalNesting += 1;
                    if x.uxCriticalNesting == 1 {
                        // TODO: portASSERT_IF_IN_ISR
                    }
                }
                None => (),
            }
            
        } else {
            mtCOVERAGE_TEST_MARKER!();
        }
    }
}

/// Exit critical section
pub fn vTaskExitCritical() {
    // ... 实现代码
    unsafe {
        if XSCHEDULERRUNNING != pdFALSE {
            match get_current_tcb() {
                Some(x) => {
                    if x.uxCriticalNesting > 0 {
                        x.uxCriticalNesting -= 1;
                        if x.uxCriticalNesting == 0 {
                            portENABLE_INTERRUPTS!();
                        } else {
                            mtCOVERAGE_TEST_MARKER!();
                        }
                    } else {
                        mtCOVERAGE_TEST_MARKER!();
                    }
                }
                None => (),
            }
        } else {
            mtCOVERAGE_TEST_MARKER!();
        }
    }
}

/// Suspend task
pub fn vTaskSuspend(xTaskToSuspend_: Option<TaskHandle_t>) {
    // ... 实现代码
    let xTaskToSuspend = prvGetTCBFromHandle(xTaskToSuspend_.as_ref()).unwrap();
    use crate::base::kernel::SUSPENDED_TASK_LIST;
    taskENTER_CRITICAL!();
    {
        //let pxTCB = xTaskToSuspend;
        // let pxTCB = prvGetTCBFromHandle(xTaskToSuspend);
        /* 从就绪/阻塞列表中删除任务并放入挂起列表中。 */

        if uxListRemove(Arc::downgrade(&xTaskToSuspend.xStateListItem)) == 0 {
            // taskRESET_READY_PRIORITY( pxTCB->uxPriority );
            //TODO:
        } else {
            mtCOVERAGE_TEST_MARKER!();
        }
        uxListRemove(Arc::downgrade(&xTaskToSuspend.xEventListItem));
        vListInsertEnd(&SUSPENDED_TASK_LIST, &xTaskToSuspend.xStateListItem);
    }
    taskEXIT_CRITICAL!();

    if (get_scheduler_running!() != false) {
        taskENTER_CRITICAL!();
        {
            prvResetNextTaskUnblockTime(); //TODO:
        }
        taskEXIT_CRITICAL!();
    } else {
        mtCOVERAGE_TEST_MARKER!();
    }
    // if ( pxTCB == pxCurrentTCB ){//TODO: pxCurrentTCB

    if is_current_tcb_raw(xTaskToSuspend) {
        if get_scheduler_running!() {
            portYIELD_WITHIN_API!();
        } else {
            if listCurrentListLength(&SUSPENDED_TASK_LIST) != get_uxCurrentNumberOfTasks!() {
            } else {
                vTaskSwitchContext();
            }
        }
    } else {
        mtCOVERAGE_TEST_MARKER!();
    }
}

/// Resume task
pub fn vTaskResume(xTaskToResume_: Option<TaskHandle_t>) {
    // ... 实现代码
    let xTaskToResume = xTaskToResume_.unwrap();
    let mut pxTCB = xTaskToResume.read();
    if is_current_tcb(Arc::downgrade(&xTaskToResume)) == false {
        taskENTER_CRITICAL!();
        {
            if prvTaskIsTaskSuspended(&xTaskToResume) != false {
                uxListRemove(Arc::downgrade(&pxTCB.xStateListItem));
                prvAddNewTaskToReadyList(&xTaskToResume);
                if (pxTCB.uxPriority >= get_current_tcb().unwrap().uxPriority) {
                    taskYIELD_IF_USING_PREEMPTION!();
                } else {
                    mtCOVERAGE_TEST_MARKER!();
                }
            } else {
                mtCOVERAGE_TEST_MARKER!();
            }
        }
        taskEXIT_CRITICAL!();
    }
}

/// Check if task is suspended
pub fn prvTaskIsTaskSuspended(xTaskToResume: &TaskHandle_t) -> bool {
    // ... 实现代码
    let mut xReturn: bool = false;
    if listIsContainedWithin(&*SUSPENDED_TASK_LIST, &xTaskToResume.read().xStateListItem) {
        if !listIsContainedWithin(&*PENDING_READY_LIST, &xTaskToResume.read().xEventListItem) {
            xReturn = true;
        }
    }
    xReturn
}

/// Get TCB from handle
pub fn prvGetTCBFromHandle(
    handle: Option<&TaskHandle_t>,
) -> Option<&'static mut tskTaskControlBlock> {
    // ... 实现代码
    match handle {
        Some(x) => unsafe {
            let temp = &*(x.read()) as *const tskTaskControlBlock;
            Some(&mut *(temp as *mut tskTaskControlBlock))
        },
        None => get_current_tcb(),
    }
}

/// Delete task
pub fn vTaskDelete(xTaskToDelete: Option<&TaskHandle_t>) {
    // ... 实现代码
    taskENTER_CRITICAL!();
    let pxTCB = prvGetTCBFromHandle(xTaskToDelete);
    uxListRemove(Arc::downgrade(&pxTCB.unwrap().xStateListItem));
    //todo：事件相关处理
    //todo：任务和tcb内存释放
    //todo：钩子函数
    taskEXIT_CRITICAL!();
    let need_yield = match xTaskToDelete {
        Some(x) => is_current_tcb(Arc::downgrade(x)),
        None => true,
    };
    if need_yield {
        portYIELD!();
    }
}

/// Suspend all tasks
#[no_mangle]
pub extern "C" fn vTaskSuspendAll() {
    // ... 实现代码
    unsafe {
        uxSchedulerSuspended += 1;
    }
}

/// Resume all tasks
#[no_mangle]
pub extern "C" fn vTaskResumeAll() -> bool {
    // ... 实现代码
    let mut xAlreadyYielded = false;
    let mut moved = false;
    unsafe {
        uxSchedulerSuspended -= 1;
        taskENTER_CRITICAL!();
        if uxSchedulerSuspended == 0 {
            if get_uxCurrentNumberOfTasks!() > 0 {
                while !listIsEmpty(&PENDING_READY_LIST) {
                    let pxTCB = listGetOwnerOfNextEntry(&PENDING_READY_LIST)
                        .upgrade()
                        .unwrap();
                    uxListRemove(Arc::downgrade(&pxTCB.read().xEventListItem));
                    uxListRemove(Arc::downgrade(&pxTCB.read().xStateListItem));

                    if pxTCB.read().uxPriority >= get_current_tcb().unwrap().uxPriority {
                        xYieldPending = true;
                    }
                    prvAddNewTaskToReadyList(&pxTCB);

                    moved = true;
                }
                if moved {
                    prvResetNextTaskUnblockTime();
                }
                let mut xPendedTicks_ = xPendedTicks;
                if xPendedTicks_ > 0 {
                    while xPendedTicks_ > 0 {
                        xTaskIncrementTick(); //todo return value

                        xPendedTicks_ -= 1;
                    }
                    xYieldPending = true;
                    xPendedTicks = 0;
                }
                if xYieldPending {
                    if cfg!(feature = "configUSE_PREEMPTION") {
                        xAlreadyYielded = true;
                    }
                    portYIELD_WITHIN_API!();
                }
            }
        }
        taskEXIT_CRITICAL!();
    }
    xAlreadyYielded
}

/// Remove task from event list
pub fn xTaskRemoveFromEventList(pxEventList: &ArcListLink) -> bool {
    // ... 实现代码
    let pxUnblockedTCB: TaskHandle_t;
    let from_c: bool;
    let test = listGetOwnerOfHeadEntryC(pxEventList);
    match test {
        Some(x) => {
            pxUnblockedTCB = x;
            from_c = true;
        }
        None => {
            pxUnblockedTCB = listGetOwnerOfHeadEntry(pxEventList).upgrade().unwrap();
            from_c = false;
        }
    }

    let xReturn: bool;
    let uxSchedulerSuspended_: UBaseType;
    unsafe {
        uxSchedulerSuspended_ = uxSchedulerSuspended;
    }
    uxListRemove(Arc::downgrade(&pxUnblockedTCB.read().xEventListItem));
    if uxSchedulerSuspended_ == 0 {
        uxListRemove(Arc::downgrade(&pxUnblockedTCB.read().xStateListItem));
        prvAddTaskToReadyList(&pxUnblockedTCB);
        if cfg!(feature = "configUSE_TICKLESS_IDLE") {
            prvResetNextTaskUnblockTime();
        }
    } else {
        vListInsertEnd(&PENDING_READY_LIST, &pxUnblockedTCB.read().xEventListItem);
    }
    if pxUnblockedTCB.read().uxPriority > get_current_tcb().unwrap().uxPriority {
        xReturn = true;
        unsafe {
            xYieldPending = true;
        }
    } else {
        xReturn = false;
    }
    if from_c {
        let temp = Arc::into_raw(pxUnblockedTCB);
    }
    xReturn
}

/// Set timeout state
pub fn vTaskInternalSetTimeOutState(pxTimeOut: &mut TimeOut) {
    // ... 实现代码
    unsafe {
        pxTimeOut.xOverflowCount = xNumOfOverflows;
        pxTimeOut.xTimeOnEntering = xTickCount;
    }
}

/// Set timeout state (public)
pub fn vTaskSetTimeOutState(pxTimeOut: &mut TimeOut) {
    // ... 实现代码
    taskENTER_CRITICAL!();
    unsafe {
        pxTimeOut.xOverflowCount = xNumOfOverflows;
        pxTimeOut.xTimeOnEntering = xTickCount;
    }
    taskEXIT_CRITICAL!();
}

/// Check for timeout
pub fn xTaskCheckForTimeOut(pxTimeOut: &mut TimeOut, pxTicksToWait: &mut TickType) -> BaseType {
    // ... 实现代码
    let xReturn: BaseType;
    taskENTER_CRITICAL!();
    {
        let xConstTickCount: TickType;
        unsafe {
            xConstTickCount = xTickCount;
        }
        let xElapsedTime = xConstTickCount - pxTimeOut.xTimeOnEntering;
        if cfg!(feature = "INCLUDE_xTaskAbortDelay") {
            //todo
        }

        if cfg!(feature = "INCLUDE_vTaskSuspend") {
            if *pxTicksToWait == PORT_MAX_DELAY {
                taskEXIT_CRITICAL!();
                return pdFALSE;
            }
        }
        let xNumOfOverflows_: BaseType;
        unsafe {
            xNumOfOverflows_ = xNumOfOverflows;
        }
        if xNumOfOverflows_ != pxTimeOut.xOverflowCount
            && xConstTickCount >= pxTimeOut.xTimeOnEntering
        {
            xReturn = pdTRUE;
            *pxTicksToWait = 0;
        } else if xElapsedTime < *pxTicksToWait {
            *pxTicksToWait -= xElapsedTime;
            xReturn = pdFALSE;
        } else {
            xReturn = pdTRUE;
            *pxTicksToWait = 0;
        }
    }
    taskEXIT_CRITICAL!();
    xReturn
}

/// Place task on event list
pub fn vTaskPlaceOnEventList(pxEventList: &ArcListLink, xTicksToWait: TickType) {
    // ... 实现代码
    vListInsert(pxEventList, &get_current_tcb().unwrap().xEventListItem);
    prvAddCurrentTaskToDelayedList(xTicksToWait, true);
}

/// Remove from unordered event list
pub fn vTaskRemoveFromUnorderedEventList(pxEventListItem: &ListItemLink, xItemValue: TickType) {
    // ... 实现代码
    listItemSetValue(
        pxEventListItem,
        xItemValue | taskEVENT_LIST_ITEM_VALUE_IN_USE,
    );
    uxListRemove(Arc::downgrade(pxEventListItem));
    let pxUnblockedTCB: TaskHandle_t =
        Weak::upgrade(&listItemGetOwner(&Arc::downgrade(pxEventListItem))).unwrap();
    uxListRemove(Arc::downgrade(&pxUnblockedTCB.read().xStateListItem));
    prvAddTaskToReadyList(&pxUnblockedTCB);
    if pxUnblockedTCB.read().uxPriority > get_current_tcb().unwrap().uxPriority {
        unsafe {
            xYieldPending = true;
        }
    }
}

/// Place on unordered event list
pub fn vTaskPlaceOnUnorderedEventList(
    pxEventList: &ArcListLink,
    xItemValue: TickType,
    xTicksToWait: TickType,
) {
    // ... 实现代码
    taskENTER_CRITICAL!();
    listItemSetValue(
        &get_current_tcb().unwrap().xEventListItem,
        xItemValue | taskEVENT_LIST_ITEM_VALUE_IN_USE,
    );
    vListInsertEnd(pxEventList, &get_current_tcb().unwrap().xEventListItem);
    prvAddCurrentTaskToDelayedList(xTicksToWait, true);
    taskEXIT_CRITICAL!();
}

/// Reset event item value
pub fn uxTaskResetEventItemValue() -> TickType {
    // ... 实现代码
    let uxReturn: TickType = listItemGetValue(&get_current_tcb().unwrap().xEventListItem);
    listItemSetValue(
        &get_current_tcb().unwrap().xEventListItem,
        configMAX_PRIORITIES - &get_current_tcb().unwrap().uxPriority,
    );
    uxReturn
}

/// Get task name
pub fn pcTaskGetName(xTaskToQuery: Option<&TaskHandle_t>) -> &str {
    // ... 实现代码
    let temp = prvGetTCBFromHandle(xTaskToQuery).unwrap();
    &temp.pcTaskName
}

/// Priority inherit
pub fn xTaskPriorityInherit(pxMutexHolder: Option<&TaskHandle_t>) -> BaseType {
    // ... 实现代码
    let mut xReturn: BaseType = pdFALSE;
    match pxMutexHolder {
        Some(pxMutexHolder_) => {
            let pxMutexHolderTCB: &mut tskTaskControlBlock = &mut pxMutexHolder_.write();
            if pxMutexHolderTCB.uxPriority < get_current_tcb().unwrap().uxPriority {
                if listItemGetValue(&pxMutexHolderTCB.xEventListItem)
                    & taskEVENT_LIST_ITEM_VALUE_IN_USE
                    == 0
                {
                    listItemSetValue(
                        &pxMutexHolderTCB.xEventListItem,
                        configMAX_PRIORITIES - pxMutexHolderTCB.uxPriority,
                    );
                }

                if listIsContainedWithin(
                    &READY_TASK_LISTS[pxMutexHolderTCB.uxPriority as usize],
                    &pxMutexHolderTCB.xStateListItem,
                ) == true
                {
                    uxListRemove(Arc::downgrade(&pxMutexHolderTCB.xStateListItem));
                    pxMutexHolderTCB.uxPriority = get_current_tcb().unwrap().uxPriority;
                    prvAddTaskToReadyList(&pxMutexHolder_);
                } else {
                    pxMutexHolderTCB.uxPriority = get_current_tcb().unwrap().uxPriority;
                }
                xReturn = pdTRUE;
            } else if pxMutexHolderTCB.uxPriority > get_current_tcb().unwrap().uxPriority {
                xReturn = pdTRUE;
            }
        }
        None => {
            mtCOVERAGE_TEST_MARKER!();
        }
    }

    xReturn
}

/// Priority disinherit
pub fn xTaskPriorityDisinherit(pxMutexHolder: Option<&TaskHandle_t>) -> BaseType {
    // ... 实现代码
    let mut xReturn: BaseType = pdFALSE;
    match pxMutexHolder {
        Some(pxMutexHolder_) => {
            let pxMutexHolderTCB: &mut tskTaskControlBlock = &mut pxMutexHolder_.write();
            pxMutexHolderTCB.uxMutexesHeld -= 1;
            if pxMutexHolderTCB.uxBasePriority != pxMutexHolderTCB.uxPriority {
                if pxMutexHolderTCB.uxMutexesHeld == 0 {
                    uxListRemove(Arc::downgrade(&pxMutexHolderTCB.xStateListItem));
                    pxMutexHolderTCB.uxPriority = pxMutexHolderTCB.uxBasePriority;
                    listItemSetValue(
                        &pxMutexHolderTCB.xEventListItem,
                        configMAX_PRIORITIES - pxMutexHolderTCB.uxPriority,
                    );
                    prvAddTaskToReadyList(&pxMutexHolder_);
                    xReturn = pdTRUE;
                } else {
                    mtCOVERAGE_TEST_MARKER!();
                }
            } else {
                mtCOVERAGE_TEST_MARKER!();
            }
        }
        None => {
            mtCOVERAGE_TEST_MARKER!();
        }
    }
    xReturn
}

/// Increment mutex held count
pub fn pvTaskIncrementMutexHeldCount() -> Option<TaskHandle_t> {
    // ... 实现代码
    match &*CURRENT_TCB.write() {
        Some(x) => {
            get_current_tcb().unwrap().uxMutexesHeld += 1;
            Some(x.clone())
        }
        None => None,
    }
}

/// Priority disinherit after timeout
pub fn vTaskPriorityDisinheritAfterTimeout(
    pxMutexHolder: Option<&TaskHandle_t>,
    uxHighestPriorityWaitingTask: UBaseType,
) {
    // ... 实现代码
    let uxPriorityToUse: UBaseType;
    let uxPriorityUsedOnEntry: UBaseType;
    match pxMutexHolder {
        Some(pxMutexHolder_) => {
            let pxMutexHolderTCB: &mut tskTaskControlBlock = &mut pxMutexHolder_.write();
            uxPriorityToUse = max(
                pxMutexHolderTCB.uxBasePriority,
                uxHighestPriorityWaitingTask,
            );
            if pxMutexHolderTCB.uxPriority != uxPriorityToUse {
                if pxMutexHolderTCB.uxMutexesHeld == 1 {
                    uxPriorityUsedOnEntry = pxMutexHolderTCB.uxPriority;
                    pxMutexHolderTCB.uxPriority = uxPriorityToUse;
                    listItemSetValue(
                        &pxMutexHolderTCB.xEventListItem,
                        configMAX_PRIORITIES - uxPriorityToUse,
                    );

                    if listIsContainedWithin(
                        &READY_TASK_LISTS[uxPriorityUsedOnEntry as usize],
                        &pxMutexHolderTCB.xStateListItem,
                    ) {
                        uxListRemove(Arc::downgrade(&pxMutexHolderTCB.xStateListItem));
                        prvAddTaskToReadyList(&pxMutexHolder_);
                    }
                }
            }
        }
        None => {}
    }
}