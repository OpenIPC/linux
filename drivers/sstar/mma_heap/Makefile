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
# Makefile for MStar MMA HEAP  drivers.
#

# general options
subdir-ccflags-y := -DSSTAR_CODING_STYLE_SANITIZE_MODULE
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/include/sys/common
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/include
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/mma_heap
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/miu
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/cpu/include
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/include/$(CONFIG_SSTAR_CHIP_NAME)
# specific options
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/staging/android/ion

# files
obj-$(CONFIG_MSTAR_MMAHEAP)     += mdrv-mma_heap.o
mdrv-mma_heap-objs                       += mdrv_mma_heap.o
