#
# Copyright (c) [2019~2020] SigmaStar Technology.
#
#
# This software is licensed under the terms of the GNU General Public
# License version 2, as published by the Free Software Foundation, and
# may be copied, distributed, and modified under those terms.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License version 2 for more details.
#

#
# Makefile for SStar USB device drivers.
#

subdir-ccflags-y := -DSSTAR_CODING_STYLE_SANITIZE_MODULE
CONFIG_SSTAR_CHIP_NAME := $(subst ",,$(CONFIG_SSTAR_CHIP_NAME))

EXTRA_CFLAGS	+= -I$(srctree)/drivers/sstar/usb/gadget/udc/usb20/include/$(CONFIG_SSTAR_CHIP_NAME)
EXTRA_CFLAGS	+= -I$(srctree)/drivers/sstar/usb/gadget/udc/usb20/include
EXTRA_CFLAGS    += -I$(srctree)/drivers/usb/gadget/udc
EXTRA_CFLAGS    += -I$(srctree)/drivers/sstar/include
EXTRA_CFLAGS    += -I$(srctree)/drivers/sstar/include/$(CONFIG_SSTAR_CHIP_NAME)

obj-$(CONFIG_USB_GADGET_SSTAR_DEVICE) = udc-msb250x.o
udc-msb250x-y :=  src/msb250x_gadget.o src/msb250x_dma.o src/msb250x_ep.o src/msb250x_udc.o
