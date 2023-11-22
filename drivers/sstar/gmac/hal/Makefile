#
# Makefile for Gmac hal.
#
CONFIG_SSTAR_CHIP_NAME := $(subst ",,$(CONFIG_SSTAR_CHIP_NAME))
# general options
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/net/ethernet/stmicro/stmmac
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/include
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/include/$(CONFIG_SSTAR_CHIP_NAME)
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/gmac
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/gmac/hal/$(CONFIG_SSTAR_CHIP_NAME)

# specific options
EXTRA_CFLAGS      +=  -DMSOS_TYPE_LINUX

# files
obj-$(CONFIG_SSTAR_GMAC) += $(CONFIG_SSTAR_CHIP_NAME)/mhal_gmac.o
