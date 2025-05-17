use core::sync::atomic::{AtomicBool, AtomicU32, AtomicUsize, Ordering};
use spin::Mutex;

/// Task priority type
pub type TaskPriority = u8;

/// Task handle type
#[derive(Debug, Clone, Copy, Eq, PartialEq)]
pub struct TaskHandle(pub *mut TaskControlBlock);

/// Tick type used for time management
pub type TickType = u32;

/// Base type used for general purpose
pub type BaseType = i32;

/// Unsigned base type
pub type UBaseType = u32;

/// Task state
#[derive(Debug, Clone, Copy, Eq, PartialEq)]
pub enum TaskState {
    Running,
    Ready,
    Blocked,
    Suspended,
    Deleted,
}

/// Task control block - main task structure
pub struct TaskControlBlock {
    /// Current task state
    pub state: Mutex<TaskState>,
    /// Task priority (0 is lowest)
    pub priority: TaskPriority,
    /// Pointer to task's stack
    pub stack_ptr: *mut u8,
    /// Size of stack in words
    pub stack_size: usize,
    /// Task name for debugging
    pub name: &'static str,
    /// Time until task is unblocked
    pub wake_time: AtomicU32,
    /// Whether the task is suspended
    pub suspended: AtomicBool,
}

/// Queue handle type
#[derive(Debug, Clone, Copy, Eq, PartialEq)]
pub struct QueueHandle(pub *mut QueueControlBlock);

/// Queue control block structure
pub struct QueueControlBlock {
    /// Queue storage area
    pub storage: *mut u8,
    /// Size of each item in the queue
    pub item_size: usize,
    /// Maximum number of items in queue
    pub length: usize,
    /// Number of items currently in queue
    pub messages_waiting: AtomicU32,
    /// Tasks waiting to send to queue
    pub tasks_waiting_to_send: Mutex<Option<TaskHandle>>,
    /// Tasks waiting to receive from queue
    pub tasks_waiting_to_receive: Mutex<Option<TaskHandle>>,
}

// Safety implementations for raw pointer types
unsafe impl Send for TaskHandle {}
unsafe impl Sync for TaskHandle {}
unsafe impl Send for QueueHandle {}
unsafe impl Sync for QueueHandle {}
