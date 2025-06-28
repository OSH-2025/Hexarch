//! Task scheduler and core scheduling logic
#[macro_use]
use super::types::*;
#[macro_use]
use super::control::*;
#[macro_use]
use super::creation::*;
extern crate alloc;
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

/// Start scheduler
#[no_mangle]
pub extern "C" fn vTaskStartScheduler() {
    // ... 实现代码
    X_ISRSTACK_
        .write()
        .resize(unsafe { CONFIG_ISR_STACK_SIZE_WORDS }, 0);
    unsafe {
        XSCHEDULERRUNNING = pdTRUE;
    }
    if cfg!(feature = "configSUPPORT_STATIC_ALLOCATION") {
        let param: Param_link = 0;
        let arch=size_of::<usize>();
        let stack2ptr: StackType_t_link = &*IDLE_STACK as *const [usize; USER_STACK_SIZE]
            as *const usize as usize
            + USER_STACK_SIZE * arch
            - arch;
        xTaskCreateStatic(
            prvIdleTask as usize,
            "idle",
            USER_STACK_SIZE as u32,
            Some(param),
            Some(stack2ptr),
            Some(&IDLE_p),
            0,
        );
    }
    set_current_tcb(Some(Arc::downgrade(&IDLE_p)));
    if x_port_start_scheduler() != pdFALSE {
        panic!("error scheduler!!!!!!");
    }
}

/// Idle task
pub fn prvIdleTask(t: *mut core::ffi::c_void) {
    // ... 实现代码
    vSendString("idle gogogogo");
    loop {
        #[cfg(feature = "configUSE_IDLE_HOOK")]
        {
            /* Call the user defined function from within the idle task.  This
             * allows the application designer to add background functionality
             * without the overhead of a separate task.
             * NOTE: vApplicationIdleHook() MUST NOT, UNDER ANY CIRCUMSTANCES,
             * CALL A FUNCTION THAT MIGHT BLOCK. */
            vApplicationIdleHook();
        } /* configUSE_IDLE_HOOK */
        vSendString("idle gogogogo!!!(in loop)");
    }
}

/// Select highest priority task
pub fn taskSELECT_HIGHEST_PRIORITY_TASK() {
    // ... 实现代码
    let max_prio = taskSELECT_HIGHEST_PRIORITY();

    let owner: WeakListItemOwnerLink = listGetOwnerOfNextEntry(&READY_TASK_LISTS[max_prio]);
    set_current_tcb(Some(owner));
    auto_set_currentTcb();
}

/// Select highest priority
pub fn taskSELECT_HIGHEST_PRIORITY() -> usize {
    // ... 实现代码
    for i in 1..16 {
        let j = 16 - i;
        if !listIsEmpty(&READY_TASK_LISTS[j]) {
            return j;
        }
    }
    return 0;
}

/// Task yield
pub fn taskYield() {
    portYIELD!();//宏调用
}

/// System tick handler
pub fn xPortSysTickHandler() {
    // ... 实现代码
    vTaskEnterCritical();
    xTaskIncrementTick();
    vTaskExitCritical();
}

/// Increment tick
#[no_mangle]
pub extern "C" fn xTaskIncrementTick() {
    // ... 实现代码
    unsafe {
        if uxSchedulerSuspended == 0 {
            xTickCount += 1;
            if xTickCount == 0 {
                taskSWITCH_DELAYED_LISTS();
            }

            if xTickCount >= xNextTaskUnblockTime {
                loop {
                    if listIsEmpty(&DELAYED_TASK_LIST) {
                        xNextTaskUnblockTime = PORT_MAX_DELAY;
                        break;
                    } else {
                        let head: ListItemLink =
                            listGetHeadEntry(&DELAYED_TASK_LIST).upgrade().unwrap();
                        if head.read().xItemValue <= xTickCount {
                            uxListRemove(Arc::downgrade(&head));
                            let owner: TaskHandle_t;
                            let temp = listItemGetOwnerC(&Arc::downgrade(&head));
                            let from_c: bool;
                            match temp {
                                Some(x) => {
                                    owner = x;
                                    from_c = true;
                                }
                                None => {
                                    owner = listItemGetOwner(&Arc::downgrade(&head))
                                        .upgrade()
                                        .unwrap();
                                    from_c = false;
                                }
                            }
                            let prio: UBaseType = owner.read().uxPriority;
                            vListInsertEnd(&READY_TASK_LISTS[prio as usize], &head);
                            if from_c {
                                let temp_ = Arc::into_raw(owner);
                            }
                        } else {
                            xNextTaskUnblockTime = head.read().xItemValue;
                            break;
                        }
                    }
                }
            }
        } else {
            xPendedTicks += 1;
        }
    }
}

/// Add current task to delayed list
pub fn prvAddCurrentTaskToDelayedList(xTicksToWait: TickType, xCanBlockIndefinitely: bool) {
    // ... 实现代码
    //vTaskEnterCritical();

    let xTimeToWake: TickType;
    let xConstTickCount: TickType;

    unsafe {
        xTimeToWake = xTicksToWait + xTickCount;
        xConstTickCount = xTickCount;
    }

    let temp = get_current_tcb().unwrap();
    let list_item = &temp.xStateListItem;

    listItemSetValue(&list_item, xTimeToWake);
    uxListRemove(Arc::downgrade(&list_item));
    if xTimeToWake > xConstTickCount {
        vListInsert(&DELAYED_TASK_LIST, &list_item);
        unsafe {
            if xTimeToWake < xNextTaskUnblockTime {
                xNextTaskUnblockTime = xTimeToWake;
            }
        }
    } else {
        vListInsert(&OVERFLOW_DELAYED_TASK_LIST, &list_item);
    }
}

/// Task delay
#[no_mangle]
pub extern "C" fn vTaskDelay(xTicksToDelay: TickType) {
    // ... 实现代码
    vTaskEnterCritical();
    prvAddCurrentTaskToDelayedList(xTicksToDelay, true);

    vTaskExitCritical();
    taskYield();
}

/// 直到指定的唤醒时间才继续运行当前任务。
#[no_mangle]
pub extern "C" fn xTaskDelayUntil(pxPreviousWakeTime: &mut TickType, xTimeIncrement: TickType) {
    // ... 实现代码
    let mut xShouldDelay: bool = false;

    vTaskSuspendAll();
    {
        let xConstTickCount: TickType;
        unsafe {
            xConstTickCount = xTickCount;
        }

        let xTimeToWake: TickType = *pxPreviousWakeTime + xTimeIncrement;
        // 判断是否需要延迟
        if xConstTickCount < *pxPreviousWakeTime {
            if (xTimeToWake < *pxPreviousWakeTime) && (xTimeToWake > xConstTickCount) {
                xShouldDelay = true;
            } else {
                mtCOVERAGE_TEST_MARKER!();
            }
        } else {
            if (xTimeToWake < *pxPreviousWakeTime) || (xTimeToWake > xConstTickCount) {
                xShouldDelay = true;
            } else {
                mtCOVERAGE_TEST_MARKER!();
            }
        }

        *pxPreviousWakeTime = xTimeToWake;

        if xShouldDelay == true {
            prvAddCurrentTaskToDelayedList(xTimeToWake - xConstTickCount, true);
        } else {
            mtCOVERAGE_TEST_MARKER!();
        }
    }
    let xAlreadyYielded: bool = vTaskResumeAll();
    if xAlreadyYielded == false {
        portYIELD_WITHIN_API!();
    } else {
        mtCOVERAGE_TEST_MARKER!();
    }
}

/// Switch delayed lists
pub fn taskSWITCH_DELAYED_LISTS() {
    // ... 实现代码
    let mut delayed = DELAYED_TASK_LIST.write();
    let mut overflowed = OVERFLOW_DELAYED_TASK_LIST.write();
    let tmp = (*delayed).clone();
    *delayed = (*overflowed).clone();
    *overflowed = tmp;
    unsafe {
        xNumOfOverflows += 1;
    }
}

/// Reset next task unblock time
pub fn prvResetNextTaskUnblockTime() {
    // ... 实现代码
    unsafe {
        if listIsEmpty(&DELAYED_TASK_LIST) {
            xNextTaskUnblockTime = PORT_MAX_DELAY;
        } else {
            let head = listGetHeadEntry(&DELAYED_TASK_LIST).upgrade().unwrap();
            xNextTaskUnblockTime = head.read().xItemValue;
        }
    }
}