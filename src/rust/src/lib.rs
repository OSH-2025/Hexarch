#![no_std]
#![feature(const_mut_refs)]
#![feature(const_fn_trait_bound)]

//! FreeRTOS Rust Implementation
//!
//! This is a Rust port of the FreeRTOS kernel, designed to be
//! memory-safe and efficient while maintaining API compatibility
//! with the original C implementation.

pub mod critical;
pub mod list;
// pub mod port;
pub mod queue;
pub mod task;

mod config;
mod types;

// Re-export commonly used types
pub use config::*;
pub use types::*;

/// Kernel version information
pub const VERSION: &str = "FreeRTOS Rust 0.1.0";

/// Error type for FreeRTOS operations
#[derive(Debug, Copy, Clone, Eq, PartialEq)]
pub enum Error {
    OutOfMemory,
    QueueFull,
    QueueEmpty,
    Timeout,
    InvalidParameter,
}

/// Result type for FreeRTOS operations
pub type Result<T> = core::result::Result<T, Error>;

// Global configuration constants
pub const CONFIG_MINIMAL_STACK_SIZE: usize = 128;
pub const CONFIG_TOTAL_HEAP_SIZE: usize = 15 * 1024; // 15KB
pub const CONFIG_MAX_PRIORITIES: u8 = 5;
pub const CONFIG_TICK_RATE_HZ: u32 = 1000;
