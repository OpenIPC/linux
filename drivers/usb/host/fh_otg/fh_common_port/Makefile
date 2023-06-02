#
# Makefile for FH_common library
#
ifneq ($(KERNELRELEASE),)

EXTRA_CFLAGS	+= -DFH_LINUX
#EXTRA_CFLAGS	+= -DDEBUG
#EXTRA_CFLAGS	+= -DFH_DEBUG_REGS
#EXTRA_CFLAGS	+= -DFH_DEBUG_MEMORY

EXTRA_CFLAGS	+= -DFH_LIBMODULE
EXTRA_CFLAGS	+= -DFH_CCLIB
EXTRA_CFLAGS	+= -DFH_CRYPTOLIB
EXTRA_CFLAGS	+= -DFH_NOTIFYLIB
EXTRA_CFLAGS	+= -DFH_UTFLIB

obj-y			 := fh_common_port_lib.o
fh_common_port_lib-objs := fh_cc.o fh_modpow.o fh_dh.o \
			    fh_crypto.o fh_notifier.o \
			    fh_common_linux.o fh_mem.o

else

ifeq ($(KDIR),)
$(error Must give "KDIR=/path/to/kernel/source" on command line or in environment)
endif

ifeq ($(ARCH),)
$(error Must give "ARCH=<arch>" on command line or in environment. Also, if \
 cross-compiling, must give "CROSS_COMPILE=/path/to/compiler/plus/tool-prefix-")
endif

ifeq ($(DOXYGEN),)
DOXYGEN		:= doxygen
endif

default:
	$(MAKE) -C$(KDIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules

docs:	$(wildcard *.[hc]) doc/doxygen.cfg
	$(DOXYGEN) doc/doxygen.cfg

tags:	$(wildcard *.[hc])
	$(CTAGS) -e $(wildcard *.[hc]) $(wildcard linux/*.[hc]) $(wildcard $(KDIR)/include/linux/usb*.h)

endif

clean:
	rm -rf *.o *.ko .*.cmd *.mod.c .*.o.d .*.o.tmp modules.order Module.markers Module.symvers .tmp_versions/
