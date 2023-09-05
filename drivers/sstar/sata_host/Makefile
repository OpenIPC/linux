#
# Makefile for SStar Sata Host device drivers.
#
CONFIG_SSTAR_CHIP_NAME := $(subst ",,$(CONFIG_SSTAR_CHIP_NAME))
ifdef CONFIG_SSTAR_PROJECT_NAME
    CONFIG_SSTAR_PROJECT_NAME := $(subst ",,$(CONFIG_SSTAR_PROJECT_NAME))
endif

# general options
EXTRA_CFLAGS      +=  -Idrivers/sstar/include
EXTRA_CFLAGS      +=  -Idrivers/sstar/include/$(CONFIG_SSTAR_CHIP_NAME)
EXTRA_CFLAGS      +=  -Idrivers/ata
EXTRA_CFLAGS      +=  -Idrivers/sstar/sata_host
EXTRA_CFLAGS      +=  -Idrivers/sstar/sata_host/$(CONFIG_SSTAR_CHIP_NAME)
EXTRA_CFLAGS      +=  -Iinclude/linux

# specific options
EXTRA_CFLAGS      +=

# files
obj-$(CONFIG_SATA_HOST_0)   = mdrv-sata-host.o

ifdef CONFIG_SSTAR_SATA_AHCI_PLATFORM_HOST
mdrv-sata-host-objs     := mdrv_sata_host_ahci.o 
mdrv-sata-host-objs     += $(CONFIG_SSTAR_CHIP_NAME)/mhal_sata_host_ahci.o
else
mdrv-sata-host-objs     := mdrv_sata_host.o
endif
mdrv-sata-host-objs     += $(CONFIG_SSTAR_CHIP_NAME)/mhal_sata_host.o

