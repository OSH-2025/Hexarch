extern crate cc;

fn main() {
	cc::Build::new()
        .file("srcC/list.c")
        .file("srcC/queue.c")
        .file("srcC/tasks.c")
        .file("srcC/timers.c")
        .file("srcC/wrapper.c")
        // .file("srcC/portable/GCC/ARM_AARCH64/port.c")
        // .file("srcC/portable/GCC/ARM_AARCH64/portASM.S")
        .file("srcC/portable/MemMang/heap_1.c")
        .compiler("gcc")
        .flag("-I").flag("srcC")
        .flag("-I").flag("srcC/include")
        // .flag("-I").flag("srcC/portable/GCC/ARM_AARCH64")
        // .flag("-mcpu=cortex-a53")
        .flag("-fpic")
        .flag("-ffreestanding")
        .flag("-O2")
        .flag("-std=gnu11")
        .target("aarch64-none-elf")
        .compile("wrapper");
        // println!("cargo:rustc-link-search=wrapper");
        println!("cargo:rustc-link-arg-bins=-T");
        println!("cargo:rustc-link-arg-bins=-o");
        println!("cargo:rustc-link-arg-bins=build/FreeRTOS.elf");
}