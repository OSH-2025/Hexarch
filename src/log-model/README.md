# FreeRTOS + log-model (Raspberry Pi)

测试前：

将log.c文件放在FreeRTOS/Source目录下

将log.h文件放在FreeRTOS/Source/include目录下

```  Makefile
# 在objects.mk中添加以下内容
OBJECTS += $(BUILD_DIR)FreeRTOS/Source/log.o
```

在当前目录下执行：

```
$ make
$ qemu-system-arm -M versatilepb -cpu arm1176 -m 128M -nographic -kernel kernel.img
```

即可开始测试

# FreeRTOS + log-model (riscv)

测试前：

将log_rv.c文件放在FreeRTOS/Source目录下(之后建议更名为log.c)

将log.h文件放在FreeRTOS/Source/include目录下(之后建议更名为log.h)

将FreeRTOS/Demo/RISC-V-Qemu-virt_GCC/main_blinky.c文件替换为文件夹里的main_blinky.c

```  Makefile
# 在FreeRTOS/Demo/RISC-V-Qemu-virt_GCC/Makefile中添加以下内容,不要忘记在 '$' 前 Tab
SRCS = main.c main_blinky.c riscv-virt.c ns16550.c \
    # ...
    $(RTOS_SOURCE_DIR)/log.c \  
    # ...

# 日志模块配置 - 可以通过命令行参数控制
# 使用方法：make ENABLE_LOG=1 或 make ENABLE_LOG=0
ENABLE_LOG ?= 1

# 只在编译目标时显示配置信息，避免在clean等操作时显示
ifneq ($(MAKECMDGOALS),clean)
ifeq ($(ENABLE_LOG), 1)
    CPPFLAGS += -DconfigUSE_LOG_MODULE=1
    $(info 编译配置: 日志模块已启用)
else
    CPPFLAGS += -DconfigUSE_LOG_MODULE=0
    $(info 编译配置: 日志模块已禁用)
endif
else
# 即使在clean时也需要设置宏，以防某些规则需要
ifeq ($(ENABLE_LOG), 1)
    CPPFLAGS += -DconfigUSE_LOG_MODULE=1
else
    CPPFLAGS += -DconfigUSE_LOG_MODULE=0
endif
endif
```

``` C
// FreeRTOS/Demo/RISC-V-Qemu-virt_GCC/FreeRTOSConfig.h
// 在FreeRTOSConfig.h中添加以下内容
/* Custom application configuration */
/* 日志模块配置 - 可通过 make ENABLE_LOG=0/1 或直接在此处设置 */
#ifndef configUSE_LOG_MODULE
    #define configUSE_LOG_MODULE    1  /* 默认启用日志 */
#endif
``` 

在当前目录下执行：

```
$ make ENABLE_LOG=1 (or ENABLE_LOG=0)
$ qemu-system-riscv32 -nographic -machine virt -net none \
  -chardev stdio,id=con,mux=on -serial chardev:con \
  -mon chardev=con,mode=readline -bios none \
  -smp 4 -kernel ./build/RTOSDemo.axf
```

即可开始测试

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