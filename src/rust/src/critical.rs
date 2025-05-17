use core::sync::atomic::{AtomicU32, Ordering};

/// Interrupt nesting counter
static INTERRUPT_NESTING: AtomicU32 = AtomicU32::new(0);

/// Enter critical section
#[inline]
pub fn enter_critical() -> CriticalSection {
    // Disable interrupts and increment nesting counter
    let prev_primask = unsafe { disable_interrupts() };
    INTERRUPT_NESTING.fetch_add(1, Ordering::SeqCst);
    CriticalSection { prev_primask }
}

/// Critical section guard
pub struct CriticalSection {
    prev_primask: u32,
}

impl Drop for CriticalSection {
    fn drop(&mut self) {
        // Decrement nesting counter and restore interrupts if zero
        if INTERRUPT_NESTING.fetch_sub(1, Ordering::SeqCst) == 1 {
            unsafe {
                restore_interrupts(self.prev_primask);
            }
        }
    }
}

/// Disable interrupts and return previous state
#[inline]
unsafe fn disable_interrupts() -> u32 {
    let mut primask: u32;
    core::arch::asm!(
        "mrs {}, PRIMASK",
        "cpsid i",
        out(reg) primask
    );
    primask
}

/// Restore interrupts to previous state
#[inline]
unsafe fn restore_interrupts(primask: u32) {
    core::arch::asm!(
        "msr PRIMASK, {}",
        in(reg) primask
    );
}

/// Check if in critical section
#[inline]
pub fn in_critical_section() -> bool {
    INTERRUPT_NESTING.load(Ordering::SeqCst) > 0
}

/// Execute closure in critical section
#[inline]
pub fn critical_section<F, R>(f: F) -> R
where
    F: FnOnce() -> R,
{
    let _cs = enter_critical();
    f()
}
