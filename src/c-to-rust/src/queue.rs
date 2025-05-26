extern crate libc;
use libc::*;

use portmacro::*;
/**
 * Type by which queues are referenced.  For example, a call to xQueueCreate
 * returns (via a pointer parameter) an xQueueHandle variable that can then
 * be used as a parameter to xQueueSend(), xQueueReceive(), etc.
 */
type xQueueHandle = *mut c_void;

/* For internal use only */
// pub const queueSEND_TO_BACK: c_int = 0;
// pub const queueSEND_TO_FRONT: c_int = 1;

#[repr(C)]
enum sendPosition{
    BACK = 0,
    FRONT = 1,
}

/* For internal use onle. These definitions *must* match those in queue.c */

// pub const queueQUEUE_TYPE_BASE:c_uint = 0;
// pub const queueQUEUE_TYPE_MUTEX:c_uint = 1;
// pub const queueQUEUE_TYPE_COUNTING_SEMAPHORE:c_uint = 2;
// pub const queueQUEUE_TYPE_BINARY_SEMAPHORE:c_uint = 3;
// pub const queueQUEUE_TYPE_RECURSIVE_MUTEX:c_uint = 4;

#[repr(C)]
enum queueType{
    BASE = 0,
    MUTEX = 1,
    COUNTING_SEMAPHORE = 2,
    BINART_SEMAPHORE = 3,
    RECURSIVE_MUTEX = 4,
}

// MEMO(宏函数: 这些宏函数本身并不是一个新的函数,而是给已经存在的函数起了一个别名,作用是方便调用,但是在rust中没有对应的功能,因此我们不需要这些宏函数,而是在改写的过程中发现C语言代码中哪里调用了这些宏函数的时候手动替换为对应的函数即可)

//MEMO(C语言中const修饰的指针:)
/*
const void *：数据不可变，指针可变（最常见）。
void *const：指针不可变，数据可变。
const void *const：指针和数据都不可变。
 */

//MEMO(有一点要说明的是这个函数的返回值在C中是显式声明的signed portBASE_TYPE, 但portBASE_TYPE本身是c_long类型,隐含了有符号,这里signed应该是为了强调有符号的属性)
#[no_mangle]
pub extern "C" 
    fn xQueueGenericSend( 
        pxQueue: xQueueHandle, 
        pvItemToQueue: *const c_void,
        xTicksToWait: portTickType,
        xCopyPosition: portBASE_TYPE
    ) -> portBASE_TYPE_SIGNED {
        //TODO
    }

#[no_mangle]
pub extern "C"
    fn xQueueGenericReceive(
        xQueue: xQueueHandle,
        pvBuffer: *mut c_void, // const修饰的指针本身不可变,但rust中不直接表达此约束, 此函数声明在queue.h line 779
        xTicksToWait: portTickType,
        xJustPeek: portBASE_TYPE,
    ) -> portBASE_TYPE_SIGNED{
        //TODO
    }
 
 #[no_mangle]
 pub extern "C"
    fn uxQueueMessagesWaiting(
        xQueue: *const c_void,
    ) -> portBASE_TYPE_UNSIGNED {
        //TODO
    }

#[no_mangle]
pub extern "C"
    fn vQueueDelete(
        pxQueue: xQueueHandle,
    ) {
        //TODO
    }

#[no_mangle]
pub extern "C"
    fn xQueueGenericSendFromISR(
        pxQueue: xQueueHandle,
        pvItemToQueue: *const c_void,
        pxHigherPriorityTaskWoken: *mut portBASE_TYPE_SIGNED,
        xCopyPosition: portBASE_TYPE,
    ) -> portBASE_TYPE_SIGNED {
        //TODO
    }

#[no_mangle]
pub extern "C"
    fn xQueueReceiveFromISR(
        pxQueue: xQueueHandle,
        pvBuffer: *mut c_void,
        pxHigherPriorityTaskWoken: *mut portBASE_TYPE_SIGNED,
    ) -> portBASE_TYPE_SIGNED {
        //TODO
    }

#[no_mangle]
pub extern "C"
    fn xQueueIsQueueEmptyFromISR(
        pxQueue: *const c_void, //这个对应关系实在是对应不上了,只能显式使用c_void
    ) -> portBASE_TYPE_SIGNED {
        //TODO
    }

#[no_mangle]
pub extern "C"
    fn xQueueIsQueueFullFromISR(
        pxQueue: *const c_void,
    ) -> portBASE_TYPE_SIGNED {
        //TODO
    }

#[no_mangle]
pub extern "C"
    fn uxQueueMessagesWaitingFromISR(
        pxQueue: *const c_void,
    ) -> portBASE_TYPE_UNSIGNED {
        //TODO
    }

#[no_mangle]
pub extern "C"
    fn xQueueAltGenericSend(
        pxQueue: xQueueHandle,
        pvItemToQueue: *const c_void,
        xTicksToWait: portTickType,
        xCopyPosition: portBASE_TYPE,
    ) -> portBASE_TYPE_SIGNED {
        //TODO
    }

#[no_mangle]
pub extern "C"
    fn xQueueAltGenericReceive(
        pxQueue: xQueueHandle, 
        pvBuffer: *mut c_void,
        xTicksToWait: portTickType,
        xJustPeek: portBASE_TYPE,
    ) -> portBASE_TYPE_SIGNED {
        //TODO
    }

#[no_mangle]
pub extern "C"
    fn xQueueCRSendFromISR(
        pxQueue: xQueueHandle,
        pvItemToQueue: *const c_void,
        xCoRoutinePreviouslyWoken: portBASE_TYPE_SIGNED,
    ) -> portBASE_TYPE_SIGNED {
        //TODO
    }

#{no_mangle}
pub extern "C"
    fn xQueueCRReceiveFromISR(
        pxQueue: xQueueHandle,
        pvBuffer: *mut c_void,
        pxTaskWoken: *mut portBASE_TYPE_SIGNED,
    ) -> portBASE_TYPE_SIGNED {
        //TODO
    }

#[no_mangle]
pub extern "C"
    fn xQueueCRSend(
        pxQueue: xQueueHandle,
        pvItemToQueue: *const c_void,
        xTicksToWait: portTickType,
    ) -> portBASE_TYPE_SIGNED {
        //TODO
    }

#[no_mangle]
pub extern "C"
    fn xQueueCRReceive(
        pxQueue: xQueueHandle,
        pvBuffer: *mut c_void,
        xTicksToWait: portTickType,
    ) -> portBASE_TYPE_SIGNED {
        //TODO
    }

    //TODO(到line 1241(未写))