extern crate libc;
use libc::*;

// portmacro里有部分类型的声明
use portmacro::*;

/*
重写的一点小建议:
1. 看到函数内部调用了某个神秘函数,先去c源码里搜一下这个函数的出处
2. if 是正常的函数,那么直接调用即可, 如果是宏定义的函数, 则需要找到对应的那个通用函数, 自行替换, 这么说有点不清不楚, 到时候线下再具体说明一下吧
*/
pub type xQueueHandle = *mut c_void;

/* For internal use only */
// pub const queueSEND_TO_BACK: c_int = 0;
// pub const queueSEND_TO_FRONT: c_int = 1;

// MEMO(这两个枚举enum我想说明一下,如果简单的按照一对一的对应关系来,应该写成上边两行注释掉的样子,即pub const xxxx = xxxx,但这么写更能体现rust的特性吧(maybe),但是这么写显然有个问题就是,我并没有一味的将名称原封不动的对应过来,而是去掉了queue前缀,见仁见智,目前先这么写了,如果实在是不方便再说吧)
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
        //author:lsy
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
        //author:lsy
    }
 
 #[no_mangle]
 pub extern "C"
    fn uxQueueMessagesWaiting(
        xQueue: *const c_void,
    ) -> portBASE_TYPE_UNSIGNED {
        //TODO
        //author:lsy
    }

#[no_mangle]
pub extern "C"
    fn vQueueDelete(
        pxQueue: xQueueHandle,
    ) {
        //TODO
        //author:lsy
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
        //author:lsy
    }

#[no_mangle]
pub extern "C"
    fn xQueueReceiveFromISR(
        pxQueue: xQueueHandle,
        pvBuffer: *mut c_void,
        pxHigherPriorityTaskWoken: *mut portBASE_TYPE_SIGNED,
    ) -> portBASE_TYPE_SIGNED {
        //TODO
        //author:yhx
    }

#[no_mangle]
pub extern "C"
    fn xQueueIsQueueEmptyFromISR(
        pxQueue: *const c_void, //这个对应关系实在是对应不上了,只能显式使用c_void
    ) -> portBASE_TYPE_SIGNED {
        //TODO
        //author:yhx
    }

#[no_mangle]
pub extern "C"
    fn xQueueIsQueueFullFromISR(
        pxQueue: *const c_void,
    ) -> portBASE_TYPE_SIGNED {
        //TODO
        //author:yhx
    }

#[no_mangle]
pub extern "C"
    fn uxQueueMessagesWaitingFromISR(
        pxQueue: *const c_void,
    ) -> portBASE_TYPE_UNSIGNED {
        //TODO
        //author:yhx
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
        //author:hmh
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
        //author:hmh
    }

#[no_mangle]
pub extern "C"
    fn xQueueCRSendFromISR(
        pxQueue: xQueueHandle,
        pvItemToQueue: *const c_void,
        xCoRoutinePreviouslyWoken: portBASE_TYPE_SIGNED,
    ) -> portBASE_TYPE_SIGNED {
        //TODO
        //author:hmh
    }

#[no_mangle]
pub extern "C"
    fn xQueueCRReceiveFromISR(
        pxQueue: xQueueHandle,
        pvBuffer: *mut c_void,
        pxTaskWoken: *mut portBASE_TYPE_SIGNED,
    ) -> portBASE_TYPE_SIGNED {
        //TODO
        //author:hmh
    }

#[no_mangle]
pub extern "C"
    fn xQueueCRSend(
        pxQueue: xQueueHandle,
        pvItemToQueue: *const c_void,
        xTicksToWait: portTickType,
    ) -> portBASE_TYPE_SIGNED {
        //TODO
        //author:ly
    }

#[no_mangle]
pub extern "C"
    fn xQueueCRReceive(
        pxQueue: xQueueHandle,
        pvBuffer: *mut c_void,
        xTicksToWait: portTickType,
    ) -> portBASE_TYPE_SIGNED {
        //TODO
        //author:ly
    }

#[no_mangle]
pub extern "C"
    fn xQueueCreateMutex(
        ucQueueType: c_uchar,
    ) -> xQueueHandle{
        //TODO
        //author:ly
    } 

#[no_mangle]
pub extern "C"
    fn xQueueCreateCountingSemaphore(
        uxCountValue: portBASE_TYPE_UNSIGNED,
        uxInitialCount: portBASE_TYPE_UNSIGNED,
    ) -> xQueueHandle {
        //TODO
        //author:ly
    }

#[no_mangle]
pub extern "C"
    fn xQueueGetMutexHolder(
        xSemaphore: xQueueHandle,
    ) -> *mut c_void{
        //TODO
        //author:wcr
    }

#[no_mangle]
pub extern "C"
    fn xQueueTakeMutexRecursive(
        pxMutex: xQueueHandle,
        xBlockTime: portTickType,
    ) -> portBASE_TYPE {
        //TODO
        //author:wcr
    }

#[no_mangle]
pub extern "C"
    fn xQueueGiveMutexRecursive(
        pxMutex: xQueueHandle,
    ) -> portBASE_TYPE {
        //TODO
        //author:wcr
    }

#[no_mangle]
pub extern "C"
    fn xQueueGenericCreate(
        uxQueueLength: portBASE_TYPE_UNSIGNED,
        uxItemSize: portBASE_TYPE_UNSIGNED,
        ucQueueType: c_uchar,
    ) -> xQueueHandle {
        //TODO
        //author:wcr
    }

// configQueue_xxxx是定义在configFreeRTOS.h里的
#[cfg(configQueue_REGISTRY_SIZE > 0)]
#[no_mangle]
pub extern "C"
    fn vQueueAddToRegistry(
        xQueue: xQueueHandle,
        pcName: *mut c_schar,
    ){
        //TODO
        //author:mwy
    }


// # 非公开 API（仅供 FreeRTOS 内部使用）
// 非公开但是一定要用no_mangle和pub extern修饰,这是为了能够给C语言调用,非公开的意思是此代码只能在queue内部调用,其他模块不能直接调用这些函数
#[no_mangle]
pub extern "C"
    fn vQueueWaitForMessageRestricted(
        pxQueue: xQueueHandle,
        xTicksToWait: portTickType,
    ){
        //TODO
        //author:mwy
    }

#[no_mangle]
pub extern "C"
    fn xQueueGenericReset(
        pxQueue: xQueueHandle,
        xNewQueue: portBASE_TYPE,
    ) -> portBASE_TYPE {
        //TODO
        //author:mwy
    }