CC = gcc
CFLAGS = -Iinclude -Wall
SRCS = main.c src/ff.c src/diskio.c src/ffsystem.c src/ffunicode.c
OBJS = $(SRCS:.c=.o)
TARGET = fatfs_test

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@

clean:
	rm -f $(OBJS) $(TARGET)

