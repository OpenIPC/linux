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

subdir-ccflags-y := -DSSTAR_CODING_STYLE_SANITIZE_MODULE
CONFIG_SSTAR_CHIP_NAME := $(subst ",,$(CONFIG_SSTAR_CHIP_NAME))

obj-$(CONFIG_MS_SPINAND) += kdrv_spinand.o

# general options

#EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/include/$(CONFIG_SSTAR_CHIP_NAME)
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/include
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/spinand/hal/$(CONFIG_SSTAR_CHIP_NAME)
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/spinand/drv/inc
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/mtd

# specific options
EXTRA_CFLAGS      +=  -DMSOS_TYPE_LINUX

ifeq ($(CONFIG_SSTAR_CHIP_NAME), infinity2)
	MHAL_SOURCE = hal/$(CONFIG_SSTAR_CHIP_NAME)/mhal_spinand.o
else
	MHAL_SOURCE = hal/mhal_spinand.o
endif

kdrv_spinand-objs := drv/mdrv_spinand.o \
                     drv/mdrv_spinand_hal.o \
                     $(MHAL_SOURCE) \
                     hal/$(CONFIG_SSTAR_CHIP_NAME)/mhal_spinand_chip_config.o
