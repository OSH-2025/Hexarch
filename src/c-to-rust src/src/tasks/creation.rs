//! Task creation and initialization

#[macro_use]
use super::types::*;
#[macro_use]
use super::control::*;
#[macro_use]
use super::scheduler::*;
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

/// Create task (static).
#[cfg(feature = "configSUPPORT_STATIC_ALLOCATION")]
pub fn xTaskCreateStatic(
    pxTaskCode: usize,
    pcName: &str,
    ulStackDepth: u32,
    pvParameters: Option<Param_link>,
    puxStackBuffer: Option<StackType_t_link>,
    pxTaskBuffer: Option<&TCB_t_link>,
    uxPriority: UBaseType,
) -> Option<TaskHandle_t> {
    // ... 实现代码
    assert!(puxStackBuffer.is_some());
    assert!(pxTaskBuffer.is_some());

    let xReturn = Arc::new(RwLock::new(tskTaskControlBlock::default()));

    let pxNewTCB: &TCB_t_link = pxTaskBuffer.unwrap();
    TCB_set_pxStack(pxNewTCB, puxStackBuffer.unwrap());

    prvInitialiseNewTask(
        pxTaskCode,
        pcName,
        ulStackDepth,
        pvParameters,
        &xReturn,
        uxPriority,
        pxNewTCB,
    );

    prvAddNewTaskToReadyList(pxNewTCB);

    Some(xReturn)
}

/// Create task (dynamic).
#[cfg(feature = "configSUPPORT_DYNAMIC_ALLOCATION")]
pub fn xTaskCreate(
    pxTaskCode: usize,
    pcName: &str,
    ulStackDepth: u32,
    pvParameters: Option<Param_link>,
    uxPriority: UBaseType,
    pxCreatedTask: Option<TaskHandle_t>,
) -> BaseType {
    // ... 实现代码
    let xReturn: BaseType = 0;
    let mut pxStack: StackType_t_link = 0;

    use core::mem;

    use alloc::alloc::Layout;
    let arch=size_of::<usize>();
    let layout = Layout::from_size_align(ulStackDepth as usize * arch, arch)
        .ok()
        .unwrap();
    let stack_ptr: *mut u8;
    unsafe {
        stack_ptr = alloc::alloc::alloc(layout);
    }
    pxStack = stack_ptr as usize + ulStackDepth as usize * arch - arch;
    // let stack: Vec<usize> = Vec::with_capacity(ulStackDepth as usize);

    let pxNewTCB: TCB_t_link = Arc::new(RwLock::new(tskTaskControlBlock::default()));
    TCB_set_pxStack(&pxNewTCB, pxStack);

    prvInitialiseNewTask(
        pxTaskCode,
        pcName,
        ulStackDepth,
        pvParameters,
        &pxCreatedTask.unwrap(),
        uxPriority,
        &pxNewTCB,
    );
    prvAddNewTaskToReadyList(&pxNewTCB);
    mem::forget(pxNewTCB);
    1
}

/// Initialize new task
pub fn prvInitialiseNewTask<'a>(
    pxTaskCode: usize,
    pcName: &'a str,
    ulStackDepth: u32,
    pvParameters: Option<Param_link>,
    pxCreatedTask: &'a TaskHandle_t,
    priority: UBaseType,
    pxNewTCB: &'a TCB_t_link,
) -> &'a TaskHandle_t {
    // ... 实现代码
    let mut pxTopOfStack: StackType_t_link = pxNewTCB.read().pxStack;
    pxTopOfStack = pxTopOfStack & (!(0x0007usize));
    let params = {
        match pvParameters {
            Some(x) => x,
            None => 0,
        }
    };
    let x: UBaseType = 0;
    //TODO: name length

    pxNewTCB.write().pcTaskName = pcName.to_string();
    pxNewTCB.write().uxPriority = priority;
    if cfg!(feature = "configUSE_MUTEXES") {
        pxNewTCB.write().uxBasePriority = priority;
        pxNewTCB.write().uxMutexesHeld = 0;
    }
    //TODO:auto init

    listItemSetOwner(&pxNewTCB.write().xStateListItem, Arc::downgrade(&pxNewTCB));
    listItemSetOwner(&pxNewTCB.write().xEventListItem, Arc::downgrade(&pxNewTCB));
    listItemSetValue(
        &pxNewTCB.write().xEventListItem,
        configMAX_PRIORITIES - priority,
    );

    unsafe {
        pxNewTCB.write().pxTopOfStack =
            pxPortInitialiseStack(pxTopOfStack as *mut _, pxTaskCode, params as *mut _) as usize;
        pxNewTCB.write().uxCriticalNesting = 0;
    }

    //TODO: return
    *pxCreatedTask.write() = (*(pxNewTCB.write())).clone();
    pxNewTCB
}

/// Add new task to ready list
pub fn prvAddNewTaskToReadyList(pxNewTCB: &TCB_t_link) {
    // ... 实现代码
    {
        //TODO:
        prvAddTaskToReadyList(&pxNewTCB);
    }
}

/// Add task to ready list
pub fn prvAddTaskToReadyList(pxNewTCB: &TCB_t_link) {
    // ... 实现代码
    let uxPriority = pxNewTCB.read().uxPriority;

    taskRECORD_READY_PRIORITY(uxPriority);
    vListInsertEnd(
        &READY_TASK_LISTS[uxPriority as usize],
        &pxNewTCB.read().xStateListItem,
    );
}

/// Record ready priority
pub fn taskRECORD_READY_PRIORITY(uxPriority: UBaseType) {
    // ... 实现代码
    //TODO
}