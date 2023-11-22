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
# Makefile for SStar cam_os_wrapper driver.
#

subdir-ccflags-y := -DSSTAR_CODING_STYLE_SANITIZE_MODULE
CONFIG_SSTAR_CHIP_NAME := $(subst ",,$(CONFIG_SSTAR_CHIP_NAME))
# general options
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/include
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/include/$(CONFIG_SSTAR_CHIP_NAME)
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/cam_os_wrapper
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/cam_os_wrapper/pub
# EXTRA_CFLAGS    +=  -Wno-sync-nand
EXTRA_CFLAGS      +=  -Wno-sync-fetch-and-nand-semantics-changed

# specific options

# files
obj-y += src/cam_os_wrapper.o src/cam_os_export.o src/cam_os_informal_idr.o

# export header files
EXPORT_H_FILES    += cam_os_wrapper.h
