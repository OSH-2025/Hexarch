extern crate libc;
use libc::*;

// portmacro里有部分类型的声明
use portmacro::*;
use list::*;

pub const tskKERNEL_VERSION_NUMBER: &str = "V7.2.0";

pub type xTaskHandle = *mut c_void;

#[repr(C)]
pub struct xTIME_OUT{
    pub xOverflowCount: portBASE_TYPE,
    pub xTimeOnEntering: portTickType,
}

pub type xTimeOutType = xTIME_OUT;

// MEMO(这些地方的关于* & mut的问题,都需要解决,所以这些参数类型定义目前都不能相信是正确的)
#[repr(C)]
pub struct xMEMORY_REGION{
    pub pvBaseAddress: *mut c_void,
    pub ulLengthInBytes: c_ulong,
    pub ulParameters: c_ulong,
}

pub type xMemoryRegion = xMEMORY_REGION;

#[repr(C)]
pub struct xTASK_PARAMETERS{
    pub pvTaskCode: pdTASK_CODE,
    pub pcName: *const c_schar,
    pub usStackDepth: c_ushort,
    pub pvParameters: *mut c_void,
    pub uxPriority: portBASE_TYPE_UNSIGNED,
    pub puxStackBuffer: portSTACK_TYPE,
    pub xRegions: [xMemoryRegion; portNUM_CONFIGURABLE_REGIONS ],
}

pub type xTaskParameters = xTASK_PARAMETERS;

pub const tskIDLE_PRIORITY: portBASE_TYPE_UNSIGNED = 0;

// MEMO(这里我也不太懂该怎么弄,这一块不知道怎麽定义)
/**
 * 如果 portYIELD() 直接对应一条汇编指令，可以使用 Rust 的内联汇编：

rust
#[inline]
pub fn taskYIELD() {
    unsafe {
        core::arch::asm!(
            "svc #0",  // 示例：ARM Cortex-M 的 SVC 指令
            options(nomem, nostack, preserves_flags)
        );
    }
}


注意：

内联汇编需要 #![feature(asm)] 特性门
汇编指令依赖于具体的架构（如 ARM、x86 等）
 */
pub const taskYIELD() = portYIELD();
pub const taskENTER_CRITICAL() = portENTER_CRITICAL();
pub const taskEXIT_CRITICAL() = portEXIT_CRITICAL();
pub const taskDISABLE_INTERRUPTS() = portDISABLE_INTERRUPTS();
pub const taskENABLE_INTERRUPTS() = portENABLE_INTERRUPTS();

pub const taskSCHEDULER_NOT_STARTED:c_int = 0;
pub const taskSCHEDULER_RUNNING:c_int = 1;
pub const taskSCHEDULER_SUSPENDED:c_int = 2;

// MEMO(PRIVILEGED_FUNCTION)
#[no_mangle]
pub extern "C"
    fn vTaskAllocateMPURegions(
        xTask: xTaskHandle,
        pxRegions: *const xMemoryRegion,
    ){
        //TODO
    }

// MEMO(PRIVILEGED_FUNCTION)
#[no_mangle]
pub extern "C"
    fn vTaskDelete(
        pxTaskToDelete: xTaskHandle,
    ){
        //TODO
    }

// MEMO(PRIVILEGED_FUNCTION)
#[no_mangle]
pub extern "C"
    fn vTaskDelay(
        xTicksToDelay: portTickType,
    ){
        //TODO
    }

// MEMO(PRIVILEGED_FUNCTION)
#[no_mangle]
pub extern "C"
    fn vTaskDelayUntil(
        pxPreviousWakeTime: *mut portTickType,
        xTimeIncrement: portTickType,
    ){
        //TODO
    }

// MEMO(PRIVILEGED_FUNCTION)
#[no_mangle]
pub extern "C"
    fn uxTaskPriorityGet(
        pxTask: xTaskHandle,
    ) -> portBASE_TYPE_UNSIGNED {
        //TODO
    }

// MEMO(PRIVILEGED_FUNCTION)
#[no_mangle]
pub extern "C"
    fn vTaskPrioritySet(
        pxTask: xTaskHandle,
        uxNewPriority: portBASE_TYPE_UNSIGNED,
    ){
        //TODO
    }

// MEMO(PRIVILEGED_FUNCTION)
#[no_mangle]
pub extern "C"
    fn vTaskSuspend(
        pxTaskToSuspend: xTaskHandle,
    ){
        //TODO
    }


// MEMO(PRIVILEGED_FUNCTION)
#[no_mangle]
pub extern "C"
    fn vTaskResume(
        pxTaskToResume: xTaskHandle,
    ){
        //TODO
    }

// MEMO(PRIVILEGED_FUNCTION)
#[no_mangle]
pub extern "C"
    fn xTaskResumeFromISR(
        pxTaskToResume: xTaskHandle,
    ) -> portBASE_TYPE{
        //TODO
    }

// MEMO(PRIVILEGED_FUNCTION)
#[no_mangle]
pub extern "C"
    fn vTaskStartScheduler(

    ){
        //TODO
    }

// MEMO(PRIVILEGED_FUNCTION)
#[no_mangle]
pub extern "C"
    fn vTaskEndScheduler(

    ){
        //TODO
    }

// MEMO(PRIVILEGED_FUNCTION)
#[no_mangle]
pub extern "C"
    fn vTaskSuspendAll(

    ){
        //TODO
    }

// MEMO(PRIVILEGED_FUNCTION)
#[no_mangle]
pub extern "C"
    fn xTaskResumeAll(
        xTask: xTaskHandle,
    ) -> portBASE_TYPE_SIGNED {
        //TODO
    }

// MEMO(PRIVILEGED_FUNCTION)
#[no_mangle]
pub extern "C"
    fn xTaskIsTaskSuspended(
        xTask: xTaskHandle,
    ) -> portBASE_TYPE_SIGNED{
        //TODO
    }

// MEMO(PRIVILEGED_FUNCTION)
#[no_mangle]
pub extern "C"
    fn xTaskGetTickCount(

    ) -> portTickType {
        //TODO
    }

// MEMO(PRIVILEGED_FUNCTION)
#[no_mangle]
pub extern "C"
    fn xTaskGetTickCountFromISR(

    ) -> portTickType{
        //TODO
    }

// MEMO(PRIVILEGED_FUNCTION)
#[no_mangle]
pub extern "C"
    fn uxTaskGetNumberOfTasks(

    ) -> portBASE_TYPE_UNSIGNED {
        //TODO
    }

// MEMO(PRIVILEGED_FUNCTION)
#[no_mangle]
pub extern "C"
    fn pcTaskGetTaskName(
        xTaskToQuery: xTaskHandle,
    ) -> *mut c_schar{
        //TODO
    }

// MEMO(PRIVILEGED_FUNCTION)
#[no_mangle]
pub extern "C"
    fn vTaskList(
        pcWriteBuffer: *mut c_schar,
    ){
        //TODO
    }

// MEMO(PRIVILEGED_FUNCTION)
#[no_mangle]
pub extern "C"
    fn vTaskGetRunTimeStats(
        pcWriteBuffer: *mut c_schar,
    ){
        //TODO
    }

// MEMO(PRIVILEGED_FUNCTION)
#[no_mangle]
pub extern "C"
    fn uxTaskGetStackHighWaterMark(
        xTask: xTaskHandle,
    ) -> portBASE_TYPE_UNSIGNED{
        //TODO
    }

// MEMO(PRIVILEGED_FUNCTION)
#[cfg(feature = configUSE_APPLICATION_TASK_TAG)]
#[no_mangle]
pub extern "C"
    fn vTaskSetApplicationTaskTag(
        xTask: xTaskHandle,
        pxHookFunction: pdTASK_HOOK_CODE,
    ){
        //TODO
    }

// MEMO(PRIVILEGED_FUNCTION)
#[cfg(feature = configUSE_APPLICATION_TASK_TAG)]
#[no_mangle]
pub extern "C"
    fn xTaskGetApplicationTaskTag(
        xTask: xTaskHandle,
    ) -> pdTASK_HOOK_CODE{
        //TODO
    }

// MEMO(PRIVILEGED_FUNCTION)
#[no_mangle]
pub extern "C"
    fn xTaskCallApplicationTaskHook(
        xTask: xTaskHandle,
        pvParameters: *mut c_void,
    ) -> portBASE_TYPE{
        //TODO
    }

#[no_mangle]
pub extern "C"
    fn xTaskGetIdleTaskHandle(

    ) -> xTaskHandle{
        //TODO
    } 

// MEMO(PRIVILEGED_FUNCTION)
#[no_mangle]
pub extern "C"
    fn vTaskIncrementTick(

    ){
        //TODO
    }

// MEMO(PRIVILEGED_FUNCTION)
#[no_mangle]
pub extern "C"
    fn vTaskPlaceOnEventList(
        pxEventList: *const xList,
        xTicksToWait: portTickType,
    ){
        //TODO
    }

// MEMO(PRIVILEGED_FUNCTION)
#[no_mangle]
pub extern "C"
    fn vTaskPlaceOnEventListRestricted(
        pxEventList: *const xList,
        xTicksToWait: portTickType,
    ){
        //TODO
    }


// MEMO(PRIVILEGED_FUNCTION)
#[no_mangle]
pub extern "C"
    fn xTaskRemoveFromEventList(
        pxEventList: *const xList,
    )  -> portBASE_TYPE_SIGNED {
        //TODO
    }

// MEMO(PRIVILEGED_FUNCTION)
#[no_mangle]
pub extern "C"
    fn vTaskSwitchContext(

    ){
        //TODO
    }

// MEMO(PRIVILEGED_FUNCTION)
#[no_mangle]
pub extern "C"
    fn xTaskGetCurrentTaskHandle(

    ) -> xTaskHandle{
        //TODO
    }

// MEMO(PRIVILEGED_FUNCTION)
#[no_mangle]
pub extern "C"
    fn vTaskSetTimeOutState(
        pxTimeOut: *mut xTimeOutType,
    ){
        //TODO
    }

// MEMO(PRIVILEGED_FUNCTION)
#[no_mangle]
pub extern "C"
    fn xTaskCheckForTimeOut(
        pxTimeOut: *mut xTimeOutType,
        pxTicksToWait: *mut portTickType,
    ) -> portBASE_TYPE {
        //TODO
    } 

// MEMO(PRIVILEGED_FUNCTION)
#[no_mangle]
pub extern "C"
    fn vTaskMissedYield(

    ){
        //TODO
    }

// MEMO(PRIVILEGED_FUNCTION)
#[no_mangle]
pub extern "C"
    fn xTaskGetSchedulerState(

    ){
        //TODO
    }

// MEMO(PRIVILEGED_FUNCTION)
#[no_mangle]
pub extern "C"
    fn vTaskPriorityInherit(
        pxMutexHolder: *mut xTaskHandle,
    ){
        //TODO
    }

// MEMO(PRIVILEGED_FUNCTION)
#[no_mangle]
pub extern "C"
    fn vTaskPriorityDisinherit(
        pxMutexHolder: *mut xTaskHandle,
    ){
        //TODO
    }

// MEMO(PRIVILEGED_FUNCTION)
#[no_mangle]
pub extern "C"
    fn xTaskGenericCreate(
        pxTaskCode: pdTASK_CODE,
        pcName: *const c_schar,
        usStackDepth: c_ushort,
        pvParameters: *mut c_void,
        puxStackBuffer: *mut portSTACK_TYPE,
        xRegions: *const xMemoryRegion,
    ) -> portBASE_TYPE_SIGNED {
        //TODO
    }

// MEMO(PRIVILEGED_FUNCTION)
#[no_mangle]
pub extern "C"
    fn uxTaskGetTaskNumber(
        xTask: xTaskHandle,
    ) -> portSTACK_TYPE{
        //TODO
    }

// MEMO(PRIVILEGED_FUNCTION)
#[no_mangle]
pub extern "C"
    fn vTaskSetTaskNumber(
        xTask: xTaskHandle,
        uxHandle: portBASE_TYPE_UNSIGNED,
    ){
        //TODO
    }
