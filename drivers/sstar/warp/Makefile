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
# Makefile for MStar Infinity2 warp device drivers.

# general options
subdir-ccflags-y := -DSSTAR_CODING_STYLE_SANITIZE_MODULE
EXTRA_CFLAGS += -I$(srctree)/drivers/sstar/include
EXTRA_CFLAGS += -I$(srctree)/drivers/sstar/warp/drv/inc
EXTRA_CFLAGS += -I$(srctree)/drivers/sstar/warp/drv/pub
EXTRA_CFLAGS += -I$(srctree)/drivers/sstar/warp/hal/inc
EXTRA_CFLAGS += -I$(srctree)/drivers/sstar/warp/hal/pub

ccflags-$(CONFIG_MS_WARP) += -DMSOS_TYPE_LINUX_KERNEL

# files
obj-$(CONFIG_MS_WARP) += mstar_warp.o
mstar_warp-y := drv/src/common/dev_warp.o \
				drv/src/common/mhal_warp.o \
                drv/src/linux/lxdrv_warp.o \
			    hal/src/hal_ceva.o \
			    hal/src/hal_clk.o \
			    hal/src/hal_warp.o
