CROSS   = riscv64-unknown-elf-
CC      = $(CROSS)gcc
OBJCOPY = $(CROSS)objcopy
ARCH    = $(CROSS)ar

# Generate GCC_VERSION in number format
GCC_VERSION = $(shell $(CC) --version | grep ^$(CROSS)gcc | sed 's/^.* //g' | awk -F. '{ printf("%d%02d%02d"), $$1, $$2, $$3 }')
GCC_VERSION_NEED_ZICSR = 110100

BUILD_DIR       = build
RTOS_SOURCE_DIR = $(abspath ../../Source)
DEMO_SOURCE_DIR = $(abspath .)
FATFS_SOURCE_DIR = $(abspath ../../FatFS)

CPPFLAGS = \
	-D__riscv_float_abi_soft \
	-DportasmHANDLE_INTERRUPT=handle_trap \
	-I . -I ../Common/include \
	-I $(RTOS_SOURCE_DIR)/include \
	-I $(RTOS_SOURCE_DIR)/portable/GCC/RISC-V \
	-I $(RTOS_SOURCE_DIR)/portable/GCC/RISC-V/chip_specific_extensions/RV32I_CLINT_no_extensions \
	-I $(FATFS_SOURCE_DIR)

CFLAGS  = -mabi=ilp32 -mcmodel=medany \
	-Wall \
	-fmessage-length=0 \
	-ffunction-sections \
	-fdata-sections \
	-fno-builtin-printf

ifeq ($(shell test $(GCC_VERSION) -ge $(GCC_VERSION_NEED_ZICSR) && echo true),true)
    CFLAGS += -march=rv32imac_zicsr
else
    CFLAGS += -march=rv32imac
endif

LDFLAGS = -nostartfiles -Tfake_rom.lds \
	-march=rv32imac -mabi=ilp32 -mcmodel=medany \
	-Xlinker --gc-sections \
	-Xlinker --defsym=__stack_size=300 \
	-Xlinker -Map=$(BUILD_DIR)/RTOSDemo.map

ifeq ($(DEBUG), 1)
    CFLAGS += -Og -ggdb3
else
    CFLAGS += -O2
endif

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

ifeq ($(PICOLIBC), 1)
    CFLAGS += --specs=picolibc.specs -DPICOLIBC_INTEGER_PRINTF_SCANF
   LDFLAGS += --specs=picolibc.specs -DPICOLIBC_INTEGER_PRINTF_SCANF
endif

# FatFS 源文件
FATFS_SRCS = \
	$(FATFS_SOURCE_DIR)/ff.c \
	$(FATFS_SOURCE_DIR)/diskio.c \
	$(FATFS_SOURCE_DIR)/ffsystem.c

SRCS = main.c main_blinky.c riscv-virt.c ns16550.c \
	$(RTOS_SOURCE_DIR)/event_groups.c \
	$(RTOS_SOURCE_DIR)/list.c \
	$(RTOS_SOURCE_DIR)/log.c \
	$(RTOS_SOURCE_DIR)/queue.c \
	$(RTOS_SOURCE_DIR)/stream_buffer.c \
	$(RTOS_SOURCE_DIR)/tasks.c \
	$(RTOS_SOURCE_DIR)/timers.c \
	$(RTOS_SOURCE_DIR)/portable/MemMang/heap_4.c \
	$(RTOS_SOURCE_DIR)/portable/GCC/RISC-V/port.c \
	$(FATFS_SRCS)

ASMS = start.S vector.S\
	$(RTOS_SOURCE_DIR)/portable/GCC/RISC-V/portASM.S

OBJS = $(SRCS:%.c=$(BUILD_DIR)/%.o) $(ASMS:%.S=$(BUILD_DIR)/%.o)
DEPS = $(SRCS:%.c=$(BUILD_DIR)/%.d) $(ASMS:%.S=$(BUILD_DIR)/%.d)

$(BUILD_DIR)/RTOSDemo.axf: $(OBJS) fake_rom.lds Makefile
	$(CC) $(LDFLAGS) $(OBJS) -o $@

$(BUILD_DIR)/%.o: %.c Makefile
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

$(BUILD_DIR)/%.o: %.S Makefile
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)

-include $(DEPS)

