# SPDX-License-Identifier: GPL-2.0
#
# Makefile for PCI Endpoint Functions
#
EXTRA_CFLAGS	+= -I$(srctree)/drivers/pci/controller/dwc
EXTRA_CFLAGS	+= -I$(srctree)/drivers/sstar/include
EXTRA_CFLAGS	+= -I$(srctree)/drivers/sstar/include/$(CONFIG_SSTAR_CHIP_NAME)
EXTRA_CFLAGS	+= -I$(srctree)/drivers/sstar/pcie
EXTRA_CFLAGS	+= -I$(srctree)/drivers/sstar/pcie/intf/drv/pub

obj-$(CONFIG_PCI_EPF_TEST)		+= pci-epf-test.o
obj-$(CONFIG_PCI_EPF_VHOST)		+= pci-epf-vhost.o
