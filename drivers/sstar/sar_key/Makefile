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

# general options
subdir-ccflags-y := -DSSTAR_CODING_STYLE_SANITIZE_MODULE
EXTRA_CFLAGS += -I$(srctree)/drivers/sstar/rtc/$(CONFIG_SSTAR_CHIP_NAME)
EXTRA_CFLAGS += -I$(srctree)/drivers/sstar/include
EXTRA_CFLAGS += -I$(srctree)/drivers/sstar/rtc/reg
#obj-$(CONFIG_MS_SARKEY)		+= adc-keys.o
obj-$(CONFIG_MS_SARKEY)		+= adc-keys.o
adc-keys-objs := drv_adc-keys.o
