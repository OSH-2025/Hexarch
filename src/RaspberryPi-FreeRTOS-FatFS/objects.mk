#
#	FreeRTOS portable layer for RaspberryPi
#
OBJECTS += $(BUILD_DIR)FreeRTOS/Source/portable/GCC/RaspberryPi/port.o
OBJECTS += $(BUILD_DIR)FreeRTOS/Source/portable/GCC/RaspberryPi/portisr.o

#
#	FreeRTOS Core
#
OBJECTS += $(BUILD_DIR)FreeRTOS/Source/croutine.o
OBJECTS += $(BUILD_DIR)FreeRTOS/Source/list.o
OBJECTS += $(BUILD_DIR)FreeRTOS/Source/queue.o
OBJECTS += $(BUILD_DIR)FreeRTOS/Source/tasks.o

#
#	Interrupt Manager & GPIO Drivers
#
OBJECTS += $(BUILD_DIR)Demo/Drivers/irq.o
OBJECTS += $(BUILD_DIR)Demo/Drivers/gpio.o

$(BUILD_DIR)FreeRTOS/Source/portable/GCC/RaspberryPi/port.o: CFLAGS += -I $(BASE)Demo/

#
#	Selected HEAP implementation for FreeRTOS.
#
OBJECTS += $(BUILD_DIR)/FreeRTOS/Source/portable/MemMang/heap_4.o

#
#	Startup and platform initialisation code.
#
OBJECTS += $(BUILD_DIR)Demo/startup.o


#
#	Main Test Program
#
OBJECTS += $(BUILD_DIR)Demo/main.o

# 检测USE_FATFS宏是否启用
CFLAGS += -include $(BASE)Demo/config.h

ifeq ($(shell grep -c "USE_FATFS 1" $(BASE)Demo/config.h), 1)
#
# FatFS - 只有当 USE_FATFS=1 时才编译这些文件
#
OBJECTS += $(BUILD_DIR)FatFS/Source/ff.o
OBJECTS += $(BUILD_DIR)FatFS/Source/diskio.o
OBJECTS += $(BUILD_DIR)FatFS/Source/ffsystem.o
OBJECTS += $(BUILD_DIR)FatFS/Source/ffunicode.o
# 定义宏以通知代码FatFS已启用
CFLAGS += -DUSE_FATFS=1
else
# 定义宏以通知代码FatFS已禁用
CFLAGS += -DUSE_FATFS=0s
