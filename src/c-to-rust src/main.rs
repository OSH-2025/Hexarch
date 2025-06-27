#![no_std]
#![no_main]
#![feature(alloc_error_handler)]
#![allow(non_snake_case)]
#![feature(box_into_inner)]
#![feature(allocator_api)]
#![feature(core_intrinsics)]
#[macro_use]
mod ffi;
#[allow(dead_code)]
#[macro_use]
mod kernel;
#[macro_use]
mod portable;
mod tests;
extern crate alloc;
use alloc::format;
use alloc::sync::Arc;
use core::arch::asm;
use core::ffi::c_void;
use core::intrinsics::size_of;
use kernel::projdefs::{pdFALSE, pdTRUE};
use kernel::{config::*, event_group::*, queue::*, semphr::*, tasks::*, *};
use lazy_static::lazy_static;
use portable::portmacro::*;
use portable::riscv_virt::*;
use spin::RwLock;
use crate::allocator::init_heap;

use crate::kernel::kernel::READY_TASK_LISTS;
use crate::linked_list::{list_is_empty, list_item_get_owner};
use alloc::sync::{Weak};

#[no_mangle]
pub extern "C" fn main() -> ! {
    main_new();
    loop {}
}

extern "C" {
    fn main_blinky() -> BaseType;
    fn test_() -> BaseType;
}

fn task_high_priority(t: *mut c_void) {
    print("high priority task running");
    let mut pxPreviousWakeTime: TickType = 0;
    
    // taskYield();
    loop {
        print("high priority task running");
        // taskYield();
        // vSendString("high priority task running ");
        xTaskDelayUntil(&mut pxPreviousWakeTime, 100);
        vSendString(&format!(
            "after delay:pxPreviousWakeTime={}",
            pxPreviousWakeTime
        ));
    }
}

fn task_low_priority(t: *mut c_void) {
    print("low priority task running");
    // taskYield();
    
    loop {
        print("low priority task running");
        // taskYield();
        // vSendString("low priority task running ");
    }
}

lazy_static! {
    pub static ref task1handler: Option<TaskHandle_t> =
        Some(Arc::new(RwLock::new(tskTaskControlBlock::default())));
    pub static ref task2handler: Option<TaskHandle_t> =
        Some(Arc::new(RwLock::new(tskTaskControlBlock::default())));
}
static mut xQueue: Option<QueueHandle_t> = None;
static mut xEvent: Option<EventGroupHandle> = None;
pub fn main_new() {
    print("=== R_FreeRTOS Starting ===");
    print("Step 1: Heap allocator initialized successfully");
    let param1: Param_link = 0;
    let param2: Param_link = 0;
    let param3: Param_link = 0;

    unsafe {
        //xQueue = Some(xQueueCreate(2, 4));
        xQueue = Some(Arc::new(RwLock::new(xSemaphoreCreateBinary!())));
        xEvent = Some(Arc::new(RwLock::new(
            EventGroupDefinition::xEventGroupCreate(),
        )));
    }
    print("Step 2: Creating tasks...");
    xTaskCreate(
        task_low_priority as usize,
        "task_low_priority",
        USER_STACK_SIZE as u32,
        Some(param1),
        2,
        Some(Arc::clone(&(task1handler.as_ref().unwrap()))),
    );
    xTaskCreate(
        task_high_priority as usize,
        "task_hign_priority ",
        USER_STACK_SIZE as u32,
        Some(param2),
        3,
        Some(Arc::clone(&(task2handler.as_ref().unwrap()))),
    );
    print("Step 2: Tasks created successfully");
    print("Checking ready lists after task creation:");
    for i in 0..16 {
        if !list_is_empty(&READY_TASK_LISTS[i]) {
            print("Priority ");
            let c = ['0' as u8 + i as u8];
            unsafe{
                print(core::str::from_utf8_unchecked(&c));
            }
            print(" ready list contains tasks");
            
            // 简化版本的列表检查，避免复杂的链表遍历
            let list = &READY_TASK_LISTS[i];
            if !list_is_empty(list) {
                print(" (not empty)");
            } else {
                print(" (empty)");
            }
        }
    }

    print("Step 3: Starting scheduler...");
    print("start scheduler");
    print("Step 4: About to call vTaskStartScheduler()...");
    vTaskStartScheduler();
    print("ERROR: vTaskStartScheduler() returned, this should never happen!");
    loop {
        panic! {"error in loop!!!!!"};
    }
}
