#
# Makefile for MStar EMAC device drivers.
#

CONFIG_MSTAR_CHIP_NAME := $(subst ",,$(CONFIG_MSTAR_CHIP_NAME))
# general options
EXTRA_CFLAGS      +=  -Idrivers/mstar/include
EXTRA_CFLAGS      +=  -Idrivers/mstar/include/$(CONFIG_MSTAR_CHIP_NAME)
EXTRA_CFLAGS      +=  -Idrivers/mstar/emac
EXTRA_CFLAGS      +=  -Idrivers/mstar/emac/hal/$(CONFIG_MSTAR_CHIP_NAME)
EXTRA_CFLAGS      +=  -Iinclude/linux

# specific options
# EXTRA_CFLAGS      +=  -DRED_LION
# files
obj-$(CONFIG_MS_EMAC)             := kdrv_emac.o
kdrv_emac-y    := mdrv_emac.o
kdrv_emac-y    += hal/$(CONFIG_MSTAR_CHIP_NAME)/mhal_emac.o