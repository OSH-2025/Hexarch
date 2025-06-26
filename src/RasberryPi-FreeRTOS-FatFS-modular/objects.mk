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
OBJECTS += $(BUILD_DIR)FreeRTOS/Source/queue_base.o
OBJECTS += $(BUILD_DIR)FreeRTOS/Source/queue_isr.o
OBJECTS += $(BUILD_DIR)FreeRTOS/Source/queue_mutex.o
OBJECTS += $(BUILD_DIR)FreeRTOS/Source/queue_private.o
OBJECTS += $(BUILD_DIR)FreeRTOS/Source/queue_registry.o
OBJECTS += $(BUILD_DIR)FreeRTOS/Source/task_create.o
OBJECTS += $(BUILD_DIR)FreeRTOS/Source/task_delay.o
OBJECTS += $(BUILD_DIR)FreeRTOS/Source/task_hook.o
OBJECTS += $(BUILD_DIR)FreeRTOS/Source/task_priority.o
OBJECTS += $(BUILD_DIR)FreeRTOS/Source/task_private.o
OBJECTS += $(BUILD_DIR)FreeRTOS/Source/task_schedule.o

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

#
# FatFS
#
OBJECTS += $(BUILD_DIR)FatFS/Source/ff.o
OBJECTS += $(BUILD_DIR)FatFS/Source/diskio.o
OBJECTS += $(BUILD_DIR)FatFS/Source/ffsystem.o
OBJECTS += $(BUILD_DIR)FatFS/Source/ffunicode.o
