/// Kernel configuration parameters

/// Minimal stack size in bytes
pub const CONFIG_MINIMAL_STACK_SIZE: usize = 128;

/// Total heap size in bytes
pub const CONFIG_TOTAL_HEAP_SIZE: usize = 15 * 1024; // 15KB

/// Maximum number of priorities
pub const CONFIG_MAX_PRIORITIES: u8 = 5;

/// System tick rate in Hz
pub const CONFIG_TICK_RATE_HZ: u32 = 1000;

/// Maximum length of task name
pub const CONFIG_MAX_TASK_NAME_LEN: usize = 16;

/// Size of idle task stack
pub const CONFIG_IDLE_TASK_STACK_SIZE: usize = CONFIG_MINIMAL_STACK_SIZE;

/// Whether to include statistics gathering code
pub const CONFIG_GENERATE_RUN_TIME_STATS: bool = false;

/// Whether to include trace facility
pub const CONFIG_USE_TRACE_FACILITY: bool = false;

/// Whether to include mutex support
pub const CONFIG_USE_MUTEXES: bool = true;

/// Whether to include recursive mutex support
pub const CONFIG_USE_RECURSIVE_MUTEXES: bool = true;

/// Whether to include counting semaphore support
pub const CONFIG_USE_COUNTING_SEMAPHORES: bool = true;

/// Whether to include queue sets
pub const CONFIG_USE_QUEUE_SETS: bool = false;

/// Whether to check stack overflow
pub const CONFIG_CHECK_FOR_STACK_OVERFLOW: bool = true;

/// Stack overflow checking method (if enabled)
pub const CONFIG_STACK_OVERFLOW_CHECK_METHOD: u8 = 1;
