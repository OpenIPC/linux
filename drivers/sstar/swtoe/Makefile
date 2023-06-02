#
# Makefile for MStar EMAC device drivers.
#

CONFIG_SSTAR_CHIP_NAME := $(subst ",,$(CONFIG_SSTAR_CHIP_NAME))
# general options
EXTRA_CFLAGS      +=  -Idrivers/sstar/include
EXTRA_CFLAGS      +=  -Idrivers/sstar
EXTRA_CFLAGS      +=  -Idrivers/sstar/include/$(CONFIG_SSTAR_CHIP_NAME)
EXTRA_CFLAGS      +=  -Idrivers/sstar/swtoe
EXTRA_CFLAGS      +=  -Iinclude/linux

# specific options
# EXTRA_CFLAGS      +=  -DRED_LION
# files
obj-$(CONFIG_SS_SWTOE)             := kdrv_swtoe.o
kdrv_swtoe-y    := mdrv_swtoe.o
kdrv_swtoe-y    += mdrv_swtoe_cb.o
kdrv_swtoe-y    += mdrv_swtoe_rxq.o
kdrv_swtoe-y    += mdrv_swtoe_txq.o
kdrv_swtoe-y    += mdrv_swtoe_prot.o
