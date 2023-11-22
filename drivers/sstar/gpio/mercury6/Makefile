#
# Makefile for MStar GPIO device drivers.
#
CONFIG_SSTAR_CHIP_NAME := $(subst ",,$(CONFIG_SSTAR_CHIP_NAME))
# general options
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/include
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/include/$(CONFIG_SSTAR_CHIP_NAME)

# specific options
EXTRA_CFLAGS      +=  -DMSOS_TYPE_LINUX

# files
obj-$(CONFIG_MS_GPIO) += mhal_gpio.o
obj-$(CONFIG_MS_GPIO) += mhal_pinmux.o
obj-$(CONFIG_MS_GPIC) += mhal_gpic.o
