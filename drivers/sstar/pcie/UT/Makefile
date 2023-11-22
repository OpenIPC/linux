#
# Makefile for MStar LDC kernel mode unit test.

obj-m       := pcie_loopback.o
KERNEL 	= /users_path/kernel/linux-4.19
#KERNEL 	= /users/alterman.lin/git/I7/kernel/linux-4.19
#KERNEL 	= /home/alterman.lin/git/I6e/alkaid_master/kernel
CC  = $(CROSS_COMPILE)gcc
APP = pcie_loopback.ko

# general options
EXTRA_CFLAGS += -I$(KERNEL)/drivers/sstar/include
EXTRA_CFLAGS += -I$(KERNEL)/drivers/sstar/edma/drv/pub
EXTRA_CFLAGS += -I$(KERNEL)/drivers/pci/controller/dwc

all:
	make modules -C $(KERNEL)  M=`pwd`
clean:
	make modules clean -C $(KERNEL)  M=`pwd`
	rm -rf $(APP)
