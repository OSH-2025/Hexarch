
CFLAGS += -march=armv6z -g -Wall -Wextra
CFLAGS += -I $(BASE)FreeRTOS/Source/portable/GCC/RaspberryPi/
CFLAGS += -I $(BASE)FreeRTOS/Source/include/
CFLAGS += -I $(BASE)FatFS/Source/
CFLAGS += -I $(BASE)Demo/Drivers/

TOOLCHAIN=arm-none-eabi-
