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

# general options
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/net/ethernet/stmicro/stmmac
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/include
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/include/$(CONFIG_SSTAR_CHIP_NAME)
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/gmac
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/gmac/hal/$(CONFIG_SSTAR_CHIP_NAME)
EXTRA_CFLAGS      +=  -I$(srctree)/include/linux

EXTRA_CFLAGS      +=  -DKERNEL_PHY=1

# specific options
# EXTRA_CFLAGS      +=  -DRED_LION
# files
obj-$(CONFIG_SSTAR_GMAC)             += /hal/
obj-$(CONFIG_SSTAR_GMAC)             += dwmac4_descs_sstar.o
obj-$(CONFIG_SSTAR_GMAC)             += dwmac4_dma_sstar.o
obj-$(CONFIG_SSTAR_GMAC)             += dwmac4_lib_sstar.o
obj-$(CONFIG_SSTAR_GMAC)             += dwmac-sstar.o
obj-$(CONFIG_SSTAR_GMAC)             += dwmac_sstar_drv_proc.o
obj-$(CONFIG_SSTAR_SNPS_GMAC_TEST)   += dwmac_sstar_test.o
