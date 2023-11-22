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
# Makefile for Camdriver dip device drivers.

subdir-ccflags-y := -DSSTAR_CODING_STYLE_SANITIZE_MODULE
MS_ARCH_NAME := $(subst ",,$(CONFIG_SSTAR_CHIP_NAME))
#----------------------- drv -----------------------
EXTRA_CFLAGS                +=  -I$(srctree)/drivers/sstar/camclk
EXTRA_CFLAGS                +=  -I$(srctree)/drivers/sstar/camclk/drv/inc
EXTRA_CFLAGS                +=  -I$(srctree)/drivers/sstar/camclk/drv/pub
#----------------------- hal -----------------------
EXTRA_CFLAGS                +=  -I$(srctree)/drivers/sstar/camclk/hal/common
EXTRA_CFLAGS                +=  -I$(srctree)/drivers/sstar/camclk/hal/$(MS_ARCH_NAME)/inc
# general options
EXTRA_CFLAGS                +=  -I$(srctree)/drivers/sstar/include
EXTRA_CFLAGS                +=  -I$(srctree)/drivers/sstar/include/$(MS_ARCH_NAME)/
# specific options
EXTRA_CFLAGS                +=

ccflags-y		+= -Dcamclk_IO_OFFSET
ifdef CONFIG_CAM_CLK_SYSFS
ccflags-y		+= -DCAM_CLK_SYSFS
endif
#--------------------- sources ---------------------
obj-$(CONFIG_CAM_CLK)   += camclk.o
camclk-y :=             \
                hal/$(MS_ARCH_NAME)/src/hal_camclk_complex.o      \
                hal/$(MS_ARCH_NAME)/src/hal_camclk_tbl.o      \
                drv/src/linux/drv_camclk_module.o\
                drv/src/linux/drv_camclk_sysfs.o\
                drv/src/linux/drv_camclk_os.o\
                drv/src/drv_camclk_impl.o\
                drv/src/drv_camclk.o\
                drv/src/drv_camclk_debug.o
