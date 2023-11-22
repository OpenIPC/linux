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
# Makefile for MStar EMAC device drivers.
#

subdir-ccflags-y := -DSSTAR_CODING_STYLE_SANITIZE_MODULE
CONFIG_SSTAR_CHIP_NAME := $(subst ",,$(CONFIG_SSTAR_CHIP_NAME))
# general options
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/include
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/include/$(CONFIG_SSTAR_CHIP_NAME)
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/emac_toe
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/emac_toe/hal/$(CONFIG_SSTAR_CHIP_NAME)
EXTRA_CFLAGS      +=  -I$(srctree)/include/linux
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/bdma/

EXTRA_CFLAGS      +=  -DKERNEL_PHY=1

# specific options
# EXTRA_CFLAGS      +=  -DRED_LION
# files
obj-$(CONFIG_MS_EMAC_TOE)             := kdrv_emac.o
kdrv_emac-y    := mdrv_emac.o
kdrv_emac-y    += hal/$(CONFIG_SSTAR_CHIP_NAME)/mhal_emac.o
