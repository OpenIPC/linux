#
# Makefile for MStar GPIO HAL drivers.
#


ifeq ($(MAKE_TYPE), MODULE_STANDALONE)
include $(TOP_DIR)/modules.mk
endif


CONFIG_MSTAR_CHIP_NAME := $(subst ",,$(CONFIG_MSTAR_CHIP_NAME))

# general options
# EXTRA_CFLAGS      +=  -Idrivers/mstar/common
EXTRA_CFLAGS      +=  -Idrivers/mstar/include
EXTRA_CFLAGS      +=  -Idrivers/mstar/gpio
EXTRA_CFLAGS      +=  -Idrivers/mstar/gpio/$(CONFIG_MSTAR_CHIP_NAME)

# files
#obj-$(CONFIG_MSTAR_GPIO)             += mhal_gpio.o
