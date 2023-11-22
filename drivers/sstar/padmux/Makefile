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
# Makefile for MStar padmux device drivers.
#
subdir-ccflags-y := -DSSTAR_CODING_STYLE_SANITIZE_MODULE
CONFIG_SSTAR_CHIP_NAME := $(subst ",,$(CONFIG_SSTAR_CHIP_NAME))
# general options
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/include
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/padmux
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/padmux/$(CONFIG_SSTAR_CHIP_NAME)
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/include/$(CONFIG_SSTAR_CHIP_NAME)

# specific options
EXTRA_CFLAGS      +=  -DMSOS_TYPE_LINUX

# files
obj-$(CONFIG_MS_PADMUX) += mdrv_padmux.o
# obj-$(CONFIG_MS_GPIO) += $(CONFIG_SSTAR_CHIP_NAME)/mhal_gpio.o
# obj-$(CONFIG_MS_GPIO) += $(CONFIG_SSTAR_CHIP_NAME)/mhal_pinmux.o
# obj-$(CONFIG_MS_SW_I2C) += ms_gpioi2c.o mdrv_sw_iic.o
