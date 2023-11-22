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
# Makefile for PCIe eDMA driver.

subdir-ccflags-y := -DSSTAR_CODING_STYLE_SANITIZE_MODULE
CHIP_NAME := $(subst ",,$(CONFIG_SSTAR_CHIP_NAME))

# general options
EXTRA_CFLAGS                +=  -I$(srctree)/drivers/sstar/include
EXTRA_CFLAGS                +=  -I$(srctree)/drivers/sstar/include/$(CHIP_NAME)
EXTRA_CFLAGS                +=  -I$(srctree)/drivers/sstar/pcie
EXTRA_CFLAGS                +=  -I$(srctree)/drivers/pci/controller/dwc
#----------------------- drv -----------------------

#----------------------- hal -----------------------
EXTRA_CFLAGS                +=  -I$(srctree)/drivers/sstar/pcie/edma/hal/common/inc
EXTRA_CFLAGS                +=  -I$(srctree)/drivers/sstar/pcie/edma/hal/$(CHIP_NAME)/inc
EXTRA_CFLAGS                +=  -I$(srctree)/drivers/sstar/pcie/intf/drv/pub/
EXTRA_CFLAGS                +=  -I$(srctree)/drivers/sstar/pcie/intf/hal/$(CHIP_NAME)/inc

obj-$(CONFIG_SSTAR_PCIEIF)     += intf/drv/src/drv_pcieif.o
obj-$(CONFIG_SSTAR_PCIEIF)     += intf/hal/$(CHIP_NAME)/src/hal_pcieif.o

obj-$(CONFIG_SSTAR_EDMA)       += edma.o
edma-objs := edma/hal/common/src/hal_edma.o \
             edma/drv/src/drv_edma.o

obj-$(CONFIG_SSTAR_PCIE)       += pcie-sstar.o
obj-$(CONFIG_SSTAR_PCIE_BENCH)       += UT/pcie_bench.o

obj-$(CONFIG_SSTAR_PCI_ENDPOINT_TEST)  += verify/rc/pci_endpoint_test.o
obj-$(CONFIG_SSTAR_PCI_EPF_TEST)       += verify/ep/pci-epf-test.o
