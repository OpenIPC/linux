# SPDX-License-Identifier: GPL-2.0
#
# USB peripheral controller drivers
#
subdir-ccflags-$(CONFIG_USB_GADGET_DEBUG)	:= -DDEBUG
subdir-ccflags-$(CONFIG_USB_GADGET_VERBOSE)	+= -DVERBOSE_DEBUG
ccflags-y				+= -I$(srctree)/drivers/usb/gadget/udc

ifeq ($(CONFIG_ARCH_SSTAR),y)
ccflags-y				+= -I$(srctree)/drivers/sstar/usb/gadget/function
endif

# add for GKI build, uapi should not leak CONFIG_XXX
ifeq ($(CONFIG_ARCH_SSTAR),y)
ccflags-y			+= -DARCH_SSTAR
endif
ifeq ($(CONFIG_WINDOWS_HELLO_SUPPORT),y)
ccflags-y			+= -DWINDOWS_HELLO_SUPPORT
endif
ifeq ($(CONFIG_UVC_VER150_SUPPORT),y)
ccflags-y			+= -DUVC_VER150_SUPPORT
endif

obj-$(CONFIG_USB_LIBCOMPOSITE)	+= libcomposite.o
libcomposite-y			:= usbstring.o config.o epautoconf.o
libcomposite-y			+= composite.o functions.o configfs.o u_f.o

obj-$(CONFIG_USB_GADGET)	+= udc/ function/ legacy/
