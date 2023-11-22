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
# Makefile for SStar MIU device drivers.
#

subdir-ccflags-y := -DSSTAR_CODING_STYLE_SANITIZE_MODULE
CONFIG_SSTAR_CHIP_NAME := $(subst ",,$(CONFIG_SSTAR_CHIP_NAME))
ifdef CONFIG_MSTAR_PROJECT_NAME
    CONFIG_MSTAR_PROJECT_NAME := $(subst ",,$(CONFIG_MSTAR_PROJECT_NAME))
endif

# general options
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/include/
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/include/$(CONFIG_SSTAR_CHIP_NAME)
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/miu
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/miu/$(CONFIG_SSTAR_CHIP_NAME)
EXTRA_CFLAGS      +=  -I$(srctree)/include/linux
# specific options
EXTRA_CFLAGS      +=


# files
obj-$(CONFIG_MSTAR_MIU)             := mdrv-miu.o
mdrv-miu-objs    := $(CONFIG_SSTAR_CHIP_NAME)/mhal_miu.o \
					$(CONFIG_SSTAR_CHIP_NAME)/mhal_bw.o \
					$(CONFIG_SSTAR_CHIP_NAME)/mhal_arb.o \
					mdrv_miu.o
ifeq ($(CONFIG_MIU_PROTECT_SYSFS),y)
					mdrv_sysfs_protect.o
endif
ifeq ($(CONFIG_MIU_SUPPORT_SYNOPSYS),y)
mdrv-miu-objs    += mdrv_sysfs_arb_rwcmd.o \
					$(CONFIG_SSTAR_CHIP_NAME)/mhal_synopsys.o \
					mdrv_sysfs_bw_rwcmd.o \
					mdrv_sysfs_synopsys.o
else

mdrv-miu-objs    += mdrv_sysfs_bw.o

ifeq ($(CONFIG_MIU_ARBITRATION),y)
ifeq ($(CONFIG_MIU_RWCLIENT),y)
mdrv-miu-objs    += mdrv_sysfs_arb_rwcmd.o
else
mdrv-miu-objs    += mdrv_sysfs_arb.o
endif
endif
endif

obj-$(CONFIG_TEST_ALL_LINUX_MMU_MAP) += test_mmu_map.o
obj-$(CONFIG_MSYS_MIU_UT) += ut/
