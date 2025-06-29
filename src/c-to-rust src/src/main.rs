#![no_std]
#![no_main]
#![feature(alloc_error_handler)]
#![allow(non_snake_case)]
#![feature(box_into_inner)]
#![feature(allocator_api)]
#![feature(core_intrinsics)]
#[macro_use]

#[allow(dead_code)]
#[macro_use]
mod base;
#[macro_use]
mod semphr;
#[macro_use]
mod portable;
mod event_group;
#[macro_use]
mod list;
use crate::list::*;
#[macro_use]
mod queue;
use crate::queue::queue::*;
#[macro_use]
mod tasks;
#[macro_use]
use crate::tasks::*;
// mod tasks;
// mod queue;
// mod list;
// mod tests;
extern crate alloc;
use alloc::format;
use alloc::sync::Arc;
use core::arch::asm;
use core::ffi::c_void;
use core::intrinsics::size_of;
use base::projdefs::{pdFALSE, pdTRUE};
use base::{FreeRTOSconfig::*, *};
// use kernel::{FreeRTOSconfig::*, event_group::*, semphr::*, *};
use semphr::semphr::*;
use event_group::event_group::*;
use lazy_static::lazy_static;
use portable::portmacro::*;
use portable::riscv_virt::*;
use spin::RwLock;

#[no_mangle]
pub extern "C" fn main() -> ! {
    main_new();
    loop {}
}

extern "C" {
    fn main_blinky() -> BaseType;
    fn test_() -> BaseType;
}

fn taskHighPriority(t: *mut c_void) {
    // let mut pxPreviousWakeTime: TickType = 0;
    // let mut count = 0;
    loop {
        // count += 1;
        // if count >= 10 {
        //     vTaskDelete(None);
        // }
        // xTaskDelayUntil(&mut pxPreviousWakeTime, 100);
        //  vSendString(&format!(
        //      "pxPreviousWakeTime={}",
        //      pxPreviousWakeTime
        //  ));
        vSendString("high priority task running ");
        // vSendString("12345678945612345678945612 ");
        
    }
}
fn taskLowPriority(t: *mut c_void) {
    // let mut pxPreviousWakeTime: TickType = 0;
    // let mut count = 0;
    loop {
        // count += 1;
        // if count >= 30 {
        //     break;
        // }
        vSendString("low priority task running ");
        // xTaskDelayUntil(&mut pxPreviousWakeTime, 200);
    }
}


lazy_static! {
    pub static ref task1handler: Option<TaskHandle_t> =
        Some(Arc::new(RwLock::new(tskTaskControlBlock::default())));
    pub static ref task2handler: Option<TaskHandle_t> =
        Some(Arc::new(RwLock::new(tskTaskControlBlock::default())));
    // pub static ref task3handler: Option<TaskHandle_t> = 
    //     Some(Arc::new(RwLock::new(tskTaskControlBlock::default())));
}
static mut xQueue: Option<QueueHandle_t> = None;
static mut xEvent: Option<EventGroupHandle> = None;
pub fn main_new() {
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

    xTaskCreate(
        taskHighPriority as usize,
        "taskHighPriority",
        USER_STACK_SIZE as u32,
        Some(param1),
        3,
        Some(Arc::clone(&(task1handler.as_ref().unwrap()))),
    );
    xTaskCreate(
        taskLowPriority as usize,
        "taskLowPriority ",
        USER_STACK_SIZE as u32,
        Some(param2),
        3,
        Some(Arc::clone(&(task2handler.as_ref().unwrap()))),
    );
    // xTaskCreate(
    //     task3 as usize,
    //     "task3",
    //     USER_STACK_SIZE as u32,
    //     Some(param3),
    //     4,
    //     Some(Arc::clone(&(task3handler.as_ref().unwrap()))),
    // );

    print("start scheduler");
    vTaskStartScheduler();
    loop {
        panic! {"error in loop!!!!!"};
    }
}
