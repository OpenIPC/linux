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

EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/include
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/usb/host/
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/usb/dwc3
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/usb/dwc3
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/include/$(CONFIG_SSTAR_CHIP_NAME)
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/usb/host/
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/usb/core/

obj-$(CONFIG_SSTAR_USB_DWC3_OF_SIMPLE) += sstar-dwc3-of-simple.o
ifeq ($(CONFIG_USB_DWC3_HOST), y)
	sstar-dwc3-of-simple-y += sstar-dwc3-debugfs.o
endif
sstar-dwc3-of-simple-y += sstar_dwc3_of_simple.o
