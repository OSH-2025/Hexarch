use crate::{types::*, Error, Result, CONFIG_MINIMAL_STACK_SIZE};
use core::{mem::MaybeUninit, ptr};
use spin::Mutex;

/// Maximum number of tasks that can be created
const MAX_TASKS: usize = 32;

/// Task scheduler state
static SCHEDULER: Mutex<Scheduler> = Mutex::new(Scheduler::new());

/// Task scheduler
pub struct Scheduler {
    /// List of all tasks
    tasks: [MaybeUninit<TaskControlBlock>; MAX_TASKS],
    /// Number of tasks created
    task_count: usize,
    /// Currently running task
    current_task: Option<TaskHandle>,
    /// Next task to run
    next_task: Option<TaskHandle>,
}

impl Scheduler {
    /// Create a new scheduler
    const fn new() -> Self {
        Self {
            tasks: unsafe { MaybeUninit::uninit().assume_init() },
            task_count: 0,
            current_task: None,
            next_task: None,
        }
    }

    /// Get the currently running task
    pub fn get_current_task(&self) -> Option<TaskHandle> {
        self.current_task
    }

    /// Schedule the next task to run
    pub fn schedule(&mut self) {
        // Simple round-robin scheduling
        if let Some(current) = self.current_task {
            let mut next_idx = 0;
            for i in 0..self.task_count {
                let task = unsafe { self.tasks[i].assume_init_ref() };
                if task.state.lock() == &TaskState::Ready && ptr::eq(task as *const _, current.0) {
                    next_idx = (i + 1) % self.task_count;
                    break;
                }
            }

            let next_task = unsafe { self.tasks[next_idx].assume_init_ref() };
            if next_task.state.lock() == &TaskState::Ready {
                self.next_task = Some(TaskHandle(next_task as *const _ as *mut _));
            }
        }
    }
}

/// Create a new task
pub fn create_task(
    entry: fn() -> (),
    name: &'static str,
    stack_size: usize,
    priority: TaskPriority,
) -> Result<TaskHandle> {
    let mut scheduler = SCHEDULER.lock();

    if scheduler.task_count >= MAX_TASKS {
        return Err(Error::OutOfMemory);
    }

    // Validate stack size
    let actual_stack_size = if stack_size < CONFIG_MINIMAL_STACK_SIZE {
        CONFIG_MINIMAL_STACK_SIZE
    } else {
        stack_size
    };

    // Allocate stack (in real implementation, this would use a proper allocator)
    let stack = unsafe {
        let layout =
            core::alloc::Layout::from_size_align(actual_stack_size, core::mem::align_of::<usize>())
                .unwrap();
        core::alloc::alloc::alloc_zeroed(layout)
    };

    if stack.is_null() {
        return Err(Error::OutOfMemory);
    }

    // Initialize task control block
    let tcb = TaskControlBlock {
        state: Mutex::new(TaskState::Ready),
        priority,
        stack_ptr: stack,
        stack_size: actual_stack_size,
        name,
        wake_time: AtomicU32::new(0),
        suspended: AtomicBool::new(false),
    };

    // Store task in scheduler
    scheduler.tasks[scheduler.task_count] = MaybeUninit::new(tcb);
    let handle = TaskHandle(unsafe {
        scheduler.tasks[scheduler.task_count].assume_init_ref() as *const _ as *mut _
    });
    scheduler.task_count += 1;

    Ok(handle)
}

/// Delete a task
pub fn delete_task(handle: TaskHandle) -> Result<()> {
    let mut scheduler = SCHEDULER.lock();

    // Find and remove task
    for i in 0..scheduler.task_count {
        let task = unsafe { scheduler.tasks[i].assume_init_ref() };
        if ptr::eq(task as *const _, handle.0) {
            // Set task state to deleted
            *task.state.lock() = TaskState::Deleted;

            // Free stack memory
            unsafe {
                let layout = core::alloc::Layout::from_size_align(
                    task.stack_size,
                    core::mem::align_of::<usize>(),
                )
                .unwrap();
                core::alloc::alloc::dealloc(task.stack_ptr, layout);
            }

            // Remove task from scheduler
            scheduler.tasks.copy_within(i + 1..scheduler.task_count, i);
            scheduler.task_count -= 1;
            return Ok(());
        }
    }

    Err(Error::InvalidParameter)
}

/// Suspend a task
pub fn suspend_task(handle: TaskHandle) -> Result<()> {
    let task = unsafe { &*handle.0 };
    task.suspended
        .store(true, core::sync::atomic::Ordering::SeqCst);
    *task.state.lock() = TaskState::Suspended;
    Ok(())
}

/// Resume a task
pub fn resume_task(handle: TaskHandle) -> Result<()> {
    let task = unsafe { &*handle.0 };
    task.suspended
        .store(false, core::sync::atomic::Ordering::SeqCst);
    *task.state.lock() = TaskState::Ready;
    Ok(())
}

/// Get current task handle
pub fn get_current_task() -> Option<TaskHandle> {
    SCHEDULER.lock().get_current_task()
}

/// Start the scheduler
pub fn start_scheduler() -> ! {
    let mut scheduler = SCHEDULER.lock();

    // Set initial task
    if scheduler.task_count > 0 {
        let first_task = unsafe { scheduler.tasks[0].assume_init_ref() };
        scheduler.current_task = Some(TaskHandle(first_task as *const _ as *mut _));
    }

    // Release lock before entering the infinite loop
    drop(scheduler);

    loop {
        // Schedule next task
        SCHEDULER.lock().schedule();

        // Context switch would happen here in a real implementation
        // For now, we just yield to the processor
        core::hint::spin_loop();
    }
}
