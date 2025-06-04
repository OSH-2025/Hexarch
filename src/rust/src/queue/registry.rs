#![no_std]
use super::*;
use super::{xQueueHandle, QueueRegistryItem};
use crate::portable::portBASE_TYPE;
use core::ffi::c_void;

// Queue registry array
static mut xQueueRegistry: [QueueRegistryItem; 8] = [QueueRegistryItem {
    pcQueueName: core::ptr::null_mut(),
    xHandle: core::ptr::null_mut(),
}; 8];

#[no_mangle]
pub extern "C" fn vQueueAddToRegistry(xQueue: xQueueHandle, pcQueueName: *mut i8) {
    let mut xIndex = 0;

    unsafe {
        // Find a free slot in the registry
        while xIndex < 8 {
            if xQueueRegistry[xIndex].xHandle.is_null() {
                xQueueRegistry[xIndex].xHandle = xQueue;
                xQueueRegistry[xIndex].pcQueueName = pcQueueName;
                break;
            }
            xIndex += 1;
        }
    }
}

#[no_mangle]
pub extern "C" fn vQueueUnregisterQueue(xQueue: xQueueHandle) {
    let mut xIndex = 0;

    unsafe {
        // Find the queue in the registry
        while xIndex < 8 {
            if xQueueRegistry[xIndex].xHandle == xQueue {
                xQueueRegistry[xIndex].xHandle = core::ptr::null_mut();
                xQueueRegistry[xIndex].pcQueueName = core::ptr::null_mut();
                break;
            }
            xIndex += 1;
        }
    }
}

#[no_mangle]
pub extern "C" fn pcQueueGetName(xQueue: xQueueHandle) -> *mut i8 {
    let mut xIndex = 0;
    let mut pcReturn = core::ptr::null_mut();

    unsafe {
        // Find the queue in the registry
        while xIndex < 8 {
            if xQueueRegistry[xIndex].xHandle == xQueue {
                pcReturn = xQueueRegistry[xIndex].pcQueueName;
                break;
            }
            xIndex += 1;
        }
    }

    pcReturn
}

#[no_mangle]
pub extern "C" fn ucQueueGetQueueNumber(xQueue: xQueueHandle) -> u8 {
    let pxQueue = xQueue as *mut QueueDefinition;
    unsafe { (*pxQueue).ucQueueType }
}

#[no_mangle]
pub extern "C" fn vQueueSetQueueNumber(xQueue: xQueueHandle, ucQueueNumber: u8) {
    let pxQueue = xQueue as *mut QueueDefinition;
    unsafe {
        (*pxQueue).ucQueueType = ucQueueNumber;
    }
}

#[no_mangle]
pub extern "C" fn ucQueueGetQueueType(xQueue: xQueueHandle) -> u8 {
    let pxQueue = xQueue as *mut QueueDefinition;
    unsafe { (*pxQueue).ucQueueType }
}
