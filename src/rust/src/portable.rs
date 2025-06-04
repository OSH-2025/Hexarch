#![no_std]

// Platform-specific type definitions
pub type portCHAR = i8;
pub type portFLOAT = f32;
pub type portDOUBLE = f64;
pub type portLONG = i32;
pub type portSHORT = i16;
pub type portSTACK_TYPE = u32;
pub type portBASE_TYPE = i32;
pub type unsigned_portBASE_TYPE = u32;
pub type portTickType = u32;

// Platform-specific constants
pub const portMAX_DELAY: portTickType = portTickType::MAX;
pub const portBYTE_ALIGNMENT: usize = 8;
pub const portBYTE_ALIGNMENT_MASK: usize = portBYTE_ALIGNMENT - 1;
pub const portNUM_CONFIGURABLE_REGIONS: usize = 1;
pub const portSTACK_GROWTH: i32 = -1;
pub const portTICK_PERIOD_MS: portTickType = 1;
pub const portINITIAL_XPSR: u32 = 0x01000000;
pub const portINITIAL_EXC_RETURN: u32 = 0xFFFFFFFD;
pub const portINITIAL_CONTROL: u32 = 0x00;
pub const portMAX_PRIGROUP_BITS: u32 = 7;
pub const portPRIGROUP_SHIFT: u32 = 8;
pub const portPRIGROUP_MASK: u32 = 0x07;
pub const portPRIGROUP_0: u32 = 0;
pub const portPRIGROUP_1: u32 = 1;
pub const portPRIGROUP_2: u32 = 2;
pub const portPRIGROUP_3: u32 = 3;
pub const portPRIGROUP_4: u32 = 4;
pub const portPRIGROUP_5: u32 = 5;
pub const portPRIGROUP_6: u32 = 6;
pub const portPRIGROUP_7: u32 = 7;

// Critical section macros
#[inline]
pub unsafe fn portENTER_CRITICAL() {
    // 保存当前中断状态并禁用中断
    let mut ulNewBASEPRI = 0;
    core::arch::asm!(
        "mrs {0}, basepri",
        "mov {1}, #0",
        "msr basepri, {1}",
        out(reg) ulNewBASEPRI,
        out(reg) _,
        options(nomem, nostack)
    );
}

#[inline]
pub unsafe fn portEXIT_CRITICAL() {
    // 恢复之前保存的中断状态
    let ulNewBASEPRI = 0;
    core::arch::asm!(
        "msr basepri, {0}",
        in(reg) ulNewBASEPRI,
        options(nomem, nostack)
    );
}

// Memory allocation functions
#[no_mangle]
pub extern "C" fn pvPortMalloc(size: usize) -> *mut core::ffi::c_void {
    // 简单的堆内存分配实现
    // 注意：这里应该使用FreeRTOS的堆实现
    let aligned_size = (size + portBYTE_ALIGNMENT_MASK) & !portBYTE_ALIGNMENT_MASK;
    let ptr = unsafe {
        core::alloc::alloc(core::alloc::Layout::from_size_align_unchecked(
            aligned_size,
            portBYTE_ALIGNMENT,
        ))
    };
    ptr as *mut core::ffi::c_void
}

#[no_mangle]
pub extern "C" fn vPortFree(ptr: *mut core::ffi::c_void) {
    if !ptr.is_null() {
        unsafe {
            core::alloc::dealloc(
                ptr as *mut u8,
                core::alloc::Layout::from_size_align_unchecked(0, portBYTE_ALIGNMENT),
            );
        }
    }
}

// Task control
static mut xSchedulerRunning: portBASE_TYPE = 0;
static mut xTickCount: portTickType = 0;

#[no_mangle]
pub extern "C" fn vTaskSuspendAll() {
    unsafe {
        xSchedulerRunning = 0;
    }
}

#[no_mangle]
pub extern "C" fn xTaskResumeAll() -> i32 {
    unsafe {
        xSchedulerRunning = 1;
        if xTickCount > 0 {
            // 处理延迟的任务
            xTickCount = 0;
            return 1;
        }
        0
    }
}

// Interrupt control
#[inline]
pub unsafe fn portDISABLE_INTERRUPTS() {
    core::arch::asm!("cpsid i", options(nomem, nostack));
}

#[inline]
pub unsafe fn portENABLE_INTERRUPTS() {
    core::arch::asm!("cpsie i", options(nomem, nostack));
}

// Yield
#[inline]
pub fn portYIELD() {
    unsafe {
        core::arch::asm!("svc 0", options(nomem, nostack));
    }
}

// Stack type
pub type StackType_t = portSTACK_TYPE;
pub type BaseType_t = portBASE_TYPE;
pub type UBaseType_t = u32;
pub type TickType_t = portTickType;

// Additional helper functions
#[inline]
pub fn portGET_HIGHEST_PRIORITY(uxTopPriority: u32, uxReadyPriorities: u32) -> u32 {
    let uxTopPriority = uxTopPriority;
    let uxReadyPriorities = uxReadyPriorities;
    let mut uxTopReadyPriority = 0;

    // 使用位操作找到最高优先级
    while (uxReadyPriorities & (1 << uxTopReadyPriority)) == 0 {
        uxTopReadyPriority += 1;
    }

    uxTopReadyPriority
}

#[inline]
pub fn portRESET_READY_PRIORITY(uxPriority: u32, uxReadyPriorities: u32) -> u32 {
    uxReadyPriorities & !(1 << uxPriority)
}

#[inline]
pub fn portGET_READY_PRIORITY(uxPriority: u32, uxReadyPriorities: u32) -> u32 {
    uxReadyPriorities | (1 << uxPriority)
}

#[inline]
pub fn portNOP() {
    unsafe {
        core::arch::asm!("nop", options(nomem, nostack));
    }
}

#[inline]
pub fn portYIELD_FROM_ISR(xHigherPriorityTaskWoken: portBASE_TYPE) {
    if xHigherPriorityTaskWoken != 0 {
        portYIELD();
    }
}

#[inline]
pub fn portSET_INTERRUPT_MASK_FROM_ISR() -> u32 {
    let ulReturn: u32;
    unsafe {
        core::arch::asm!(
            "mrs {0}, basepri",
            out(reg) ulReturn,
            options(nomem, nostack)
        );
        core::arch::asm!("mov r0, #0", "msr basepri, r0", options(nomem, nostack));
    }
    ulReturn
}

#[inline]
pub fn portCLEAR_INTERRUPT_MASK_FROM_ISR(ulNewMaskValue: u32) {
    unsafe {
        core::arch::asm!(
            "msr basepri, {0}",
            in(reg) ulNewMaskValue,
            options(nomem, nostack)
        );
    }
}
