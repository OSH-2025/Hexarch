# FreeRTOS + FatFS

在当前目录下执行：

```
$ make
$ qemu-system-arm -M versatilepb -cpu arm1176 -m 128M -nographic -kernel kernel.img
```

即可测试 FatFS 功能是否正确，预期输出如下：

```
Hello from FreeRTOS with FatFS!
FatFS Task Started
Formatting disk...
Format successful
Mounting filesystem...
Filesystem mounted successfully
FAT Type: FAT12
Creating test file...
File written successfully
Reading file content...
File content: Hello from FreeRTOS and FatFS integration!

FatFS integration test completed successfully!
```

一些更改：

1. 在 RaspberryPi-FreeRTOS 的基础上，增添了 FatFS/Source/ 核心文件，在 objects.mk 中添加了 FatFS 相关源文件，在 dbuild.config.mk 中添加了 FatFS 包含路径。

2. 原始代码中使用了 `UART0_BASE 0x101f1000`，这是 versatilepb 平台的 UART 地址；
但同时，GPIO 代码使用的是 `0x20200000`，这是树莓派的 GPIO 地址，而不是 versatilepb 的。因此对 Demo/Drivers/gpio.c 做出部分修改， 使用虚拟 GPIO 实现替代直接硬件访问，使代码可以在 versatilepb 模拟器上运行，同时保持原有 API 不变。

3. 对于 FatFS 的磁盘 I/O 层，使用静态分配 RAM 磁盘作为存储介质，更改了 diskio.c 的部分内容，同时更改了 ffconf.h 的部分宏定义以便调试；在 Demo/main.c 中设置了 fatfs_task 等函数，更改主函数来进行 FatFS 的功能测试。

4. ……

---

原版本是在树莓派机子上跑的，本版本修改了 startup.s 和 rasberrypi.ld ，可以在 qemu 运行

# FreeRTOS Ported to Raspberry Pi

This provides a very basic port of FreeRTOS to Raspberry pi.

## Howto Build

Type make! -- If you get an error then:

    $ cd .dbuild/pretty
    $ chmod +x *.py

Currently the makefile expect an arm-none-eabi- toolchain in the path. Either export the path to yours or
modify the TOOLCHAIN variable in dbuild.config.mk file.

You may also need to modify the library locations in the Makefile:

    kernel.elf: LDFLAGS += -L"c:/yagarto/lib/gcc/arm-none-eabi/4.7.1/" -lgcc
    kernel.elf: LDFLAGS += -L"c:/yagarto/arm-none-eabi/lib/" -lc

The build system also expects find your python interpreter by using /usr/bin/env python,
if this doesn't work you will get problems.

To resolve this, modify the #! lines in the .dbuild/pretty/*.py files.

Hope this helps.

I'm currently porting my BitThunder project to the Pi, which is a OS based on FreeRTOS
but with a comprehensive driver model, and file-systems etc.

http://github.com/jameswalmsley/bitthunder/

James

