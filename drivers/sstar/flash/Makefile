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

obj-$(CONFIG_SS_NAND_ONEBIN) += kdrv_spinand.o
obj-$(CONFIG_SS_SOCECC) += fcie/
obj-$(CONFIG_SS_NOR_ONEBIN)  += kdrv_spinor.o

# general options

EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/bdma/$(CONFIG_SSTAR_CHIP_NAME)
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/include
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/flash/
ifeq ($(CONFIG_SS_NAND_ONEBIN),y)
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/flash/nand/
else
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/flash/nor/
endif
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/flash/os/
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/fsp_qspi/drv/inc
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/mtd
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/flash/fcie/$(CONFIG_SSTAR_CHIP_NAME)

# specific options
EXTRA_CFLAGS      +=  -DMSOS_TYPE_LINUX
EXTRA_CFLAGS      +=  -DENABLE_NAND_ONEBIN

kdrv_spinand-objs := nand/mdrvSpinandBbtBbm.o \
                     nand/drvSPINAND.o  \
                     nand/spinand.o \
                     os/mdrvFlashOsImpl.o

kdrv_spinor-objs  := nor/drvSPINOR.o  \
                     nor/mtd_serflash.o \
                     os/mdrvFlashOsImpl.o
