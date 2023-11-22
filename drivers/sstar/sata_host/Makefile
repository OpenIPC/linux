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
# Makefile for SStar Sata Host device drivers.
#
subdir-ccflags-y := -DSSTAR_CODING_STYLE_SANITIZE_MODULE
CONFIG_SSTAR_CHIP_NAME := $(subst ",,$(CONFIG_SSTAR_CHIP_NAME))
ifdef CONFIG_SSTAR_PROJECT_NAME
    CONFIG_SSTAR_PROJECT_NAME := $(subst ",,$(CONFIG_SSTAR_PROJECT_NAME))
endif

# general options
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/include
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/include/$(CONFIG_SSTAR_CHIP_NAME)
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/ata
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/sata_host
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/sata_host/$(CONFIG_SSTAR_CHIP_NAME)
EXTRA_CFLAGS      +=  -I$(srctree)/include/linux

# specific options
EXTRA_CFLAGS      +=

# files
obj-$(CONFIG_ATA)         += mdrv_sata_common.o

# sata0 files
obj-$(CONFIG_SATA_HOST_0)   += mdrv-sata-host.o
mdrv-sata-host-objs     := mdr_sata_host_ahci_platform.o
mdrv-sata-host-objs     += $(CONFIG_SSTAR_CHIP_NAME)/mhal_sata_host_ahci.o
mdrv-sata-host-objs     += $(CONFIG_SSTAR_CHIP_NAME)/mhal_sata_host.o

# sata1 files
obj-$(CONFIG_SATA_HOST_1)   += mdrv-sata-host1.o
mdrv-sata-host1-objs     := mdr_sata_host_ahci_platform1.o
mdrv-sata-host1-objs     += $(CONFIG_SSTAR_CHIP_NAME)/mhal_sata_host_ahci.o
mdrv-sata-host1-objs     += $(CONFIG_SSTAR_CHIP_NAME)/mhal_sata_host.o

obj-$(CONFIG_SS_SATA_BENCH_TEST) += bench_test/

obj-$(CONFIG_SS_SATA_BRING_UP)   += bringup_script/sata_ut.o
