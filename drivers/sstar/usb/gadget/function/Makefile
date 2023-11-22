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
# USB peripheral controller drivers
#

ccflags-y			:= -I$(srctree)/drivers/usb/gadget/
ccflags-y			+= -I$(srctree)/drivers/usb/gadget/udc/

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

# USB Functions
obj-$(CONFIG_USB_U_SSTAR_AUDIO)	+= u_audio.o
usb_f_uac1-y			:= f_uac1.o
obj-$(CONFIG_USB_F_SSTAR_UAC1)	+= usb_f_uac1.o
usb_f_uvc-y			:= f_uvc.o uvc_queue.o uvc_v4l2.o uvc_video.o uvc_configfs.o
obj-$(CONFIG_USB_F_SSTAR_UVC)		+= usb_f_uvc.o
usb_f_dfu-y			:= f_dfu.o
obj-$(CONFIG_USB_F_SSTAR_DFU)	+= usb_f_dfu.o

subdir-ccflags-y := -DSSTAR_CODING_STYLE_SANITIZE_MODULE
