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

CONFIG_SSTAR_CHIP_NAME := $(subst ",,$(CONFIG_SSTAR_CHIP_NAME))
ifdef CONFIG_SSTAR_PROJECT_NAME
    CONFIG_SSTAR_PROJECT_NAME := $(subst ",,$(CONFIG_SSTAR_PROJECT_NAME))
endif


# general options

EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/include
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/include/$(CONFIG_SSTAR_CHIP_NAME)
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/noe/drv/eth
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/noe/hal/$(CONFIG_SSTAR_CHIP_NAME)
EXTRA_CFLAGS      +=  -I$(srctree)/include/linux
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/noe/drv/nat/hw_nat


ifeq ($(CONFIG_NOE_QDMA),y)
EXTRA_CFLAGS      +=  -DCONFIG_RAETH_QDMA
endif

ifeq ($(CONFIG_NOE_GMAC2),y)
EXTRA_CFLAGS      +=  -DCONFIG_RAETH_GMAC2
EXTRA_CFLAGS      +=  -DCONFIG_PSEUDO_SUPPORT
endif
EXTRA_CFLAGS      +=  -DCONFIG_RA_NAT_HW


obj-$(CONFIG_NOE_HW_NAT) += hw_nat.o
hw_nat-objs := mdrv_hwnat.o
hw_nat-objs += mdrv_hwnat_foe.o
hw_nat-objs += mdrv_hwnat_util.o
hw_nat-objs += mdrv_hwnat_ioctl.o
hw_nat-objs += mdrv_hwnat_log.o
hw_nat-objs += mdrv_hwnat_api.o


hw_nat-objs += mdrv_hwnat_mcast.o
hw_nat-objs += ../../../hal/$(CONFIG_SSTAR_CHIP_NAME)/mhal_hwnat.o

