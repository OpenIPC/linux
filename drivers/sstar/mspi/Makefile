#
# Makefile for kernel SPI drivers.
#
CONFIG_SSTAR_CHIP_NAME := $(subst ",,$(CONFIG_SSTAR_CHIP_NAME))

EXTRA_CFLAGS      +=  -Idrivers/sstar/include
EXTRA_CFLAGS      +=  -Idrivers/sstar/movedma
EXTRA_CFLAGS      +=  -Idrivers/sstar/mspi/$(CONFIG_SSTAR_CHIP_NAME)
EXTRA_CFLAGS      +=  -Idrivers/sstar/include/$(CONFIG_SSTAR_CHIP_NAME)

# SPI master controller drivers (bus)
obj-$(CONFIG_SS_MSPI) += sstar_mspi.o
sstar_mspi-objs := drv_mspi.o $(CONFIG_SSTAR_CHIP_NAME)/hal_mspi.o

