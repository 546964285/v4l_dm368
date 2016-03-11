# Makefile for fbdev applications
# Change the kernel patch to point to Linux source tree
CROSS_COMPILE = arm-none-linux-gnueabi-
KERNEL_DIR=/home/andy/RidgeRun-SDK-DM36x-Turrialba-Eval/kernel/src
INSTALL_DIR1=/home/andy/targetfs/rootfs_ridgerun
INSTALL_DIR2=/home/andy/targetfs/ti_root/home/root
CMEMLIB=cmem.a470uC
CC = $(CROSS_COMPILE)gcc -I$(KERNEL_DIR)/include
TARGET=v4l2_dm368_r3

$(TARGET): $(TARGET).o
	$(CC) -o  $(TARGET) $^ $(CMEMLIB)

%.o:%.c
	$(CC) $(CFLAGS) -c $^

install1:
	cp $(TARGET) $(INSTALL_DIR1)

install2:
	sudo cp $(TARGET) $(INSTALL_DIR2)

clean:
	rm -f *.o *~ $(TARGET)
