extern crate libc;
use libc::*;


fn main() {
    println!("Hello, world!");
    let x:c_int = 0;
    println!("{}",x);
}
