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

# SPDX-License-Identifier: GPL-2.0
#
# USB gadget drivers
#

ccflags-y			:= -I$(srctree)/drivers/usb/gadget/
ccflags-y			+= -I$(srctree)/drivers/usb/gadget/udc/
ccflags-y			+= -I$(srctree)/drivers/sstar/usb/gadget/function/
ccflags-y			+= -I$(srctree)/drivers/usb/gadget/function/

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

g_sstar_gadget-y		:= sstar_gadget.o

ifeq ($(CONFIG_USB_SSTAR_UVC),y)
g_sstar_gadget-y		+= bind_uvc.o
endif

ifeq ($(CONFIG_USB_SSTAR_UAC1),y)
g_sstar_gadget-y		+= bind_uac1.o
endif

ifeq ($(CONFIG_USB_SSTAR_RNDIS),y)
g_sstar_gadget-y		+= bind_rndis.o
endif

ifeq ($(CONFIG_USB_SSTAR_DFU),y)
g_sstar_gadget-y		+= bind_dfu.o
endif

obj-$(CONFIG_USB_G_SSTAR_GADGET)		+= g_sstar_gadget.o

subdir-ccflags-y := -DSSTAR_CODING_STYLE_SANITIZE_MODULE
