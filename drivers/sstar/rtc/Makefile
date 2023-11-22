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

EXTRA_CFLAGS += -I$(srctree)/drivers/sstar/include
EXTRA_CFLAGS += -I$(srctree)/drivers/sstar/rtc/$(CONFIG_SSTAR_CHIP_NAME)
EXTRA_CFLAGS += -I$(srctree)/drivers/sstar/include/$(CONFIG_SSTAR_CHIP_NAME)
# files
obj-$(CONFIG_RTCPWC_INNER) += sstar_rtcpwc.o
sstar_rtcpwc-objs := drv_rtcpwc.o $(CONFIG_SSTAR_CHIP_NAME)/hal_rtcpwc.o
