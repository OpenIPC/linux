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

obj-$(CONFIG_MS_FLASH_ISP)           := kdrv_spinor.o

# general options
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/flash_isp
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/flash_isp/include
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/flash_isp/$(CONFIG_SSTAR_CHIP_NAME)
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/include
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/include/$(CONFIG_SSTAR_CHIP_NAME)
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/bdma/$(CONFIG_SSTAR_CHIP_NAME)

# specific options
EXTRA_CFLAGS      +=  -DMSOS_TYPE_LINUX

# files
kdrv_spinor-objs := drvDeviceInfo.o
kdrv_spinor-objs += part_mxp.o
kdrv_spinor-objs += mxp_flash.o
kdrv_spinor-objs += drvSERFLASH.o
kdrv_spinor-objs += $(CONFIG_SSTAR_CHIP_NAME)/halSERFLASH.o
kdrv_spinor-objs += mtd_serflash.o
