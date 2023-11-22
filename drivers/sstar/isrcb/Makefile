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
# Makefile for interrupt router driver
#
subdir-ccflags-y := -DSSTAR_CODING_STYLE_SANITIZE_MODULE
CONFIG_SSTAR_CHIP_NAME := $(subst ",,$(CONFIG_SSTAR_CHIP_NAME))
# general options
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/cam_os_wrapper/pub
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/isrcb/drv/pub

# files
obj-$(CONFIG_SS_ISP_ISRCB) := isrcb.o
isrcb-y := drv/src/common/drv_isrcb.o\
           drv/src/linux/drv_isrcb_module.o
