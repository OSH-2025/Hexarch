#![no_std]
#![no_main]

use freertos::{
    critical,
    queue::Queue,
    task::{self, TaskHandle},
};

// Global queue for communication between tasks
static mut QUEUE: Option<Queue> = None;

#[no_mangle]
pub extern "C" fn main() -> ! {
    // Initialize queue
    unsafe {
        QUEUE = Some(Queue::new(5, core::mem::size_of::<u32>()).unwrap());
    }

    // Create producer task
    let _producer = task::create_task(producer_task, "producer", 256, 2).unwrap();

    // Create consumer task
    let _consumer = task::create_task(consumer_task, "consumer", 256, 1).unwrap();

    // Start the scheduler
    task::start_scheduler()
}

fn producer_task() {
    let mut counter: u32 = 0;
    loop {
        // Critical section to access global queue
        critical::critical_section(|| {
            if let Some(queue) = unsafe { &QUEUE } {
                // Send counter value to queue
                let bytes = counter.to_le_bytes();
                if queue.send(&bytes, Some(10)).is_ok() {
                    counter = counter.wrapping_add(1);
                }
            }
        });

        // Simulate some work
        for _ in 0..1000 {
            core::hint::spin_loop();
        }
    }
}

fn consumer_task() {
    let mut buffer = [0u8; 4];
    loop {
        // Critical section to access global queue
        critical::critical_section(|| {
            if let Some(queue) = unsafe { &QUEUE } {
                // Receive value from queue
                if queue.receive(&mut buffer, Some(10)).is_ok() {
                    let value = u32::from_le_bytes(buffer);
                    // Process received value (in real code, do something with it)
                    let _ = value;
                }
            }
        });

        // Simulate some work
        for _ in 0..1000 {
            core::hint::spin_loop();
        }
    }
}

// Panic handler
#[panic_handler]
fn panic(_info: &core::panic::PanicInfo) -> ! {
    loop {
        core::hint::spin_loop();
    }
}
