extern crate libc;

use libc::*;

// portmacro 
// #[warn(non_camel_case_types)]
type portCHAR =         c_char;
type portFLOAT =        c_float;
type portDOUBLE =       c_double;
type portLONG =         c_long;
type portSHORT =        c_short;
type portSTACK_TYPE =   c_ulong;
type portBASE_TYPE =    portLONG;
type portBASE_TYPE_SIGNED = portBASE_TYPE;
type portBASE_TYPE_UNSIGNED = c_ulong;

#[cfg(feature = "configUSE_16_BIT_TICKS")]
type portTickType =     c_ushort;

#[cfg(feature = "configUSE_16_BIT_TICKS")]
pub const portMAX_DELAY:portTickType = 0xffff;

#[cfg(not(feature = "configUSE_16_BIT_TICKS"))]
type portTickType =     c_ulong;
#[cfg(not(feature = "configUSE_16_BIT_TICKS"))]
pub const portMAX_DELAY:portTickType = 0xffffffff;

pub const portSTACK_GROWTH:c_int = -1;
//configTICK_RATE_HZ在configFreeRTOS.h里定义
pub const portTICK_RATE_MS:portTickType = 1000 / configTICK_RATE_HZ;
pub const portBYTE_ALIGNMENT:c_int = 8;

// FIXME(这里有点不太对,我不知道怎麽实现,先放在这里,对应的是portmacro.h的106行)
type portNOP() = _asm volatile ("NOP");

// TODO(下边还有好几个宏函数,目前不知道干嘛的,先不写)