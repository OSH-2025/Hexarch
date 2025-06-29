#![allow(warnings)]
extern crate cc;
use std::env;
fn main() {
    cc::Build::new()
            .compiler("riscv64-unknown-elf-gcc")
            .flag("-w")
            .file("src/srcC/portASM.S")
            .file("src/start.S")
            .flag("-march=rv32imac_zicsr")
            .file("src/srcC/wrapper.c")
            .file("src/srcC/main_blinky.c")
            .file("src/srcC/riscv-virt.c")
            .compile("portASM");
}
