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
# Makefile for MStar Infinity3 IVE device drivers.

subdir-ccflags-y := -DSSTAR_CODING_STYLE_SANITIZE_MODULE
CONFIG_SSTAR_CHIP_NAME := $(subst ",,$(CONFIG_SSTAR_CHIP_NAME))
# general options
#EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/ceva_linkdrv
#EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/include
#EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/gpio
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/include
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/bdma/$(CONFIG_SSTAR_CHIP_NAME)/


#ccflags-$(CONFIG_SSTAR_CEVAXM6)		+= -DMSOS_TYPE_LINUX_KERNEL

# files
#obj-$(CONFIG_SSTAR_CEVAXM6) += ceva_link.o
#ceva_link-y += ceva_link.o

#a
#ceva_link-y := ceva_linkdrv_xm6.o   \
#               ceva_linkdrv.o    \
#               protected_mem_db.o \
#               hal_ceva.o \
#               hal_timer.o \
#               hal_cpm.o \
#               boot_loader.o \
#               ceva_linkdrv_proc.o \
#               file_access.o

obj-$(CONFIG_SSTAR_CEVAXM6) += ceva_linkdrv_xm6.o  \
        ceva_linkdrv.o    \
        protected_mem_db.o \
        hal_ceva.o \
        hal_timer.o \
        hal_cpm.o \
        boot_loader.o \
        ceva_linkdrv_proc.o \
        file_access.o
