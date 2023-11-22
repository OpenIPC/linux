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
# Makefile for MStar Infinity3 IVE device drivers.

subdir-ccflags-y := -DSSTAR_CODING_STYLE_SANITIZE_MODULE
IVE_CHIP_DIR  := $(subst ",,$(CONFIG_SSTAR_CHIP_NAME))

# general options
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/ive
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/include

ccflags-$(CONFIG_MS_IVE)		+= -DMSOS_TYPE_LINUX_KERNEL

ifeq ($(CONFIG_SSTAR_CHIP_NAME), "infinity2")
    EXTRA_CFLAGS    += -DCONFIG_CHIP_INFINITY2
endif

ifeq ($(CONFIG_SSTAR_CHIP_NAME), "infinity6e")
    EXTRA_CFLAGS    += -DCONFIG_CHIP_INFINITYI6E
endif

# files
obj-$(CONFIG_MS_IVE) += mstar_ive.o
mstar_ive-y := mdrv_ive.o   \
               hal_ive.o    \
               hal_clk.o    \
               drv_ive.o    \
			   hal_ive_simulate.o
