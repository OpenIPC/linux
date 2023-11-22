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

EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/include
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/usb/phy/include
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/include/$(CONFIG_SSTAR_CHIP_NAME)

obj-$(CONFIG_SS_USB3_PHY) += usbpll.o usb3-phy.o

usbpll-y += clk_sstar_usbpll.o
usb3-phy-y += phy_sstar_u3phy.o phy_sstar_utmi.o phy_sstar_pipe.o phy_sstar_port.o phy_sstar_pipe_debugfs.o phy_sstar_utmi_debugfs.o

obj-$(CONFIG_SS_USB2_PHY) += phy-sstar-u2phy.o

