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
obj-$(CONFIG_MS_SDMMC)      += kdrv_sdmmc.o

EXTRA_CFLAGS                += -I$(srctree)/drivers/sstar/include
EXTRA_CFLAGS                += -I$(srctree)/drivers/sstar/include/$(CONFIG_SSTAR_CHIP_NAME)
EXTRA_CFLAGS                += -I$(srctree)/drivers/sstar/sdmmc/inc
EXTRA_CFLAGS                += -I$(srctree)/drivers/sstar/sdmmc/inc/$(CONFIG_SSTAR_CHIP_NAME)
EXTRA_CFLAGS                += -I$(srctree)/drivers/mmc/core/
EXTRA_CFLAGS                += -I$(srctree)/drivers/mmc/host

kdrv_sdmmc-y                += ms_sdmmc_lnx.o
kdrv_sdmmc-y                += src/hal_card_regs.o src/hal_card_timer.o
kdrv_sdmmc-y                += src/hal_card_intr_v5.o
kdrv_sdmmc-y                += src/hal_sdmmc_v5.o
kdrv_sdmmc-y                += src/$(subst ",,$(CONFIG_SSTAR_CHIP_NAME))/hal_card_platform.o
kdrv_sdmmc-$(CONFIG_SUPPORT_UT_VERIFY) += ms_sdmmc_drv.o
kdrv_sdmmc-$(CONFIG_SUPPORT_UT_VERIFY) += ms_sdmmc_verify.o
