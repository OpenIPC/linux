#
# Makefile for FH_otg Highspeed USB controller driver
#

ifneq ($(KERNELRELEASE),)

# Use the BUS_INTERFACE variable to compile the software for either 
# PCI(PCI_INTERFACE) or LM(LM_INTERFACE) bus.
ifeq ($(BUS_INTERFACE),)
	BUS_INTERFACE = -DPCI_INTERFACE
#	BUS_INTERFACE = -DLM_INTERFACE
endif

#EXTRA_CFLAGS	+= -DDEBUG 

# Use one of the following flags to compile the software in host-only or
# device-only mode.

ifeq ($(CONFIG_FH_HOST_ONLY)_$(CONFIG_FH_DEVICE_ONLY), y_y)
$(error "FH_HOST_ONLY FH_DEVICE_ONLY should be one of them or none!!!")
endif

ifneq ($(CONFIG_FH_HOST_ONLY),)
EXTRA_CFLAGS        += -DFH_HOST_ONLY
endif
ifneq ($(CONFIG_FH_DEVICE_ONLY),)
EXTRA_CFLAGS        += -DFH_DEVICE_ONLY
endif

EXTRA_CFLAGS	+= -Dlinux -DFH_HS_ELECT_TST
#EXTRA_CFLAGS	+= -DFH_EN_ISOC
EXTRA_CFLAGS   	+= -I$(PWD)/../fh_common_port
#EXTRA_CFLAGS   	+= -I$(PORTLIB)
EXTRA_CFLAGS   	+= -DFH_LINUX
EXTRA_CFLAGS   	+= $(CFI)
EXTRA_CFLAGS	+= $(BUS_INTERFACE)
#EXTRA_CFLAGS	+= -DFH_DEV_SRPCAP

obj-$(CONFIG_USB_FH_OTG):= fh_otg.o

fh_otg-objs	:= fh_otg_driver.o fh_otg_attr.o 
fh_otg-objs	+= fh_otg_cil.o fh_otg_cil_intr.o 
fh_otg-objs	+= fh_otg_pcd_linux.o fh_otg_pcd.o fh_otg_pcd_intr.o 
fh_otg-objs	+= fh_otg_hcd.o fh_otg_hcd_linux.o fh_otg_hcd_intr.o fh_otg_hcd_queue.o fh_otg_hcd_ddma.o
fh_otg-objs	+= fh_otg_adp.o
ifneq ($(CFI),)
fh_otg-objs	+= fh_otg_cfi.o
endif

else

PWD		:= $(shell pwd)
PORTLIB		:= $(PWD)/../fh_common_port

# Command paths
CTAGS		:= $(CTAGS)
DOXYGEN		:= $(DOXYGEN)

default: portlib
	$(MAKE) -C$(KDIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules
	
install: default
	$(MAKE) -C$(KDIR) M=$(PORTLIB) modules_install
	$(MAKE) -C$(KDIR) M=$(PWD) modules_install	

portlib:
	$(MAKE) -C$(KDIR) M=$(PORTLIB) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules
	cp $(PORTLIB)/Module.symvers $(PWD)/
	
docs:	$(wildcard *.[hc]) doc/doxygen.cfg
	$(DOXYGEN) doc/doxygen.cfg

tags:	$(wildcard *.[hc])
	$(CTAGS) -e $(wildcard *.[hc]) $(wildcard linux/*.[hc]) $(wildcard $(KDIR)/include/linux/usb*.h)


clean:
	rm -rf   *.o *.ko .*cmd *.mod.c .tmp_versions Module.symvers

endif
