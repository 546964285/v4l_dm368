# Makefile for fbdev applications
# Change the kernel patch to point to Linux source tree
CROSS_COMPILE = arm-none-linux-gnueabi-
KERNEL_DIR=/home/andy/RidgeRun-SDK-DM36x-Turrialba-Eval/kernel/src
INSTALL_DIR=/home/andy/targetfs/rootfs_ridgerun
CMEMLIB=cmem.a470uC
CC = $(CROSS_COMPILE)gcc -I$(KERNEL_DIR)/include
TARGET=v4l2_dm368

$(TARGET): $(TARGET).o
	$(CC) -o  $(TARGET) $^

%.o:%.c
	$(CC) $(CFLAGS) -c $^

install:
	cp $(TARGET) $(INSTALL_DIR)

clean:
	rm -f *.o *~ $(TARGET)
