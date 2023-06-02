obj-m       := linux_ktest.o
linux_ktest-objs := ../src/cam_os_wrapper.o cam_os_wrapper_linux_kernel_test.o

export ARCH=arm
export CROSS_COMPILE=arm-linux-gnueabihf-

KERNEL = ../../../../
CC  = $(CROSS_COMPILE)gcc
APP = cam_os_wrapper_linux_ktest
EXTRA_CFLAGS := -Idrivers/mstar/cam_os_wrapper/pub -Idrivers/mstar/include/

all:
	make modules -C $(KERNEL)  M=`pwd`
	#$(CROSS_COMPILE)strip -g cam_os_wrapper_linux_ktest.ko
	#$(CC) $(APP).c -o $(APP)

clean:
	make modules clean -C $(KERNEL)  M=`pwd`
	rm -rf $(APP)