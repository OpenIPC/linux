obj-m       := linux_ktest.o
linux_ktest-objs := ../src/cam_drv_buffer.o cam_drv_buffer_linux_kernel_test.o

export ARCH=arm
export CROSS_COMPILE=arm-linux-gnueabihf-

KERNEL = $(PWD)/../../../../
CC  = $(CROSS_COMPILE)gcc
APP = cam_drv_buffer_test
EXTRA_CFLAGS := -I$(KERNEL)/drivers/sstar/include/
USER_CFLAGS := -Wall -Werror -g -O2 -ffunction-sections -funwind-tables -fstack-protector -I../pub/ -I$(KERNEL)/drivers/sstar/include/
USER_LDFLAGS := -L$(PWD)/../.build/lib -lcam_drv_buffer

all:
	make modules -C $(KERNEL)  M=`pwd`
	#$(CROSS_COMPILE)strip -g cam_drv_buffer_linux_ktest.ko
	$(CC) $(USER_CFLAGS) $(USER_LDFLAGS) $(APP).c -o $(APP)

clean:
	make modules clean -C $(KERNEL)  M=`pwd`
	rm -rf $(APP)
