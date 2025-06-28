//! Task-related type definitions and basic structures

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

// 类型别名
pub type StackType_t = usize;
pub type StackType_t_link = usize;
pub type Param_link = usize;
pub type TCB_t_link = Arc<RwLock<TCB_t>>;
pub type UBaseType_t = usize;
pub type TaskFunction_t = *mut fn(*mut c_void);
pub type tskTCB = tskTaskControlBlock;
pub type TCB_t = tskTCB;
pub type TaskHandle_t = Arc<RwLock<tskTaskControlBlock>>;

// 任务状态枚举
#[derive(Debug, Clone, Copy)]
pub enum eTaskState {
    eRunning = 0,
    eReady = 1,
    eBlocked = 2,
    eSuspended = 3,
    eDeleted = 4,
    eInvalid = 5,
}

// 超时结构
#[derive(Default)]
pub struct TimeOut {
    pub xOverflowCount: BaseType,
    pub xTimeOnEntering: TickType,
}

// 任务控制块结构
#[derive(Debug, Clone)]
pub struct tskTaskControlBlock {
    /// Stack top pointer
    pub pxTopOfStack: StackType_t_link,
    /// Stack bottom pointer
    pub pxStack: StackType_t_link,  // 改为公有
    /// Task name
    pub pcTaskName: String,  // 改为公有
    /// Task status list pointer
    pub xStateListItem: ListItemLink,
    /// Task event list pointer
    pub xEventListItem: ListItemLink,
    ///
    pub uxCriticalNesting: UBaseType_t,
    /// Task priority
    pub uxPriority: UBaseType,
    pub uxMutexesHeld: UBaseType,
    pub uxBasePriority: UBaseType,
    /// mark for ffi
    pub build_from_c: bool,
}

impl Default for tskTaskControlBlock {
    fn default() -> Self {
        tskTaskControlBlock {
            pxStack: 0,
            pxTopOfStack: 0,
            pcTaskName: String::new(),
            xStateListItem: Default::default(),
            xEventListItem: Default::default(),
            uxCriticalNesting: 0,
            uxPriority: 0,
            uxBasePriority: 0,
            uxMutexesHeld: 0,
            build_from_c: false,
        }
    }
}

// 常量
pub const taskEVENT_LIST_ITEM_VALUE_IN_USE: UBaseType = 0x8000;

// 全局变量
pub static tskIDLE_PRIORITY: UBaseType = 0;
pub static mut XSCHEDULERRUNNING: BaseType = pdFALSE;
pub static mut xTickCount: UBaseType = 0;
pub static mut xNumOfOverflows: BaseType = 0;
pub static mut xNextTaskUnblockTime: UBaseType = PORT_MAX_DELAY;
pub static mut uxCurrentNumberOfTasks: UBaseType = 0;
pub static mut uxSchedulerSuspended: UBaseType = 0;
pub static mut xPendedTicks: UBaseType = 0;
pub static mut xYieldPending: bool = false;

pub static mut xSchedulerRunning: bool = false;
/// Set pxStack of target tcb.
pub fn TCB_set_pxStack(tcb: &TCB_t_link, item: StackType_t_link) {
    tcb.write().pxStack = item;
}

extern "C" {
    /// initialise task stack space ( Extern C )
    pub fn pxPortInitialiseStack(
        pxTopOfStack: *mut StackType_t,
        pxCode: usize,
        pvParameters: *mut c_void,
    ) -> *mut StackType_t;
}