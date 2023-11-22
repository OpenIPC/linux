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
# Makefile for MStar device drivers.
#

# noe
subdir-ccflags-y := -DSSTAR_CODING_STYLE_SANITIZE_MODULE
obj-$(CONFIG_NOE) += drv/eth/

ifeq ($(CONFIG_NOE_NAT_HW),y)
obj-$(CONFIG_NOE_NAT_HW)  += drv/nat/hook/
obj-m         += drv/nat/hw_nat/
endif


