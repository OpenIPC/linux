#
# Makefile for MStar GPIO device drivers.
#
CONFIG_MSTAR_CHIP_NAME := $(subst ",,$(CONFIG_MSTAR_CHIP_NAME))
# general options
##EXTRA_CFLAGS      +=  -Idrivers/mstar/common
EXTRA_CFLAGS      +=  -Idrivers/mstar/include
EXTRA_CFLAGS      +=  -Idrivers/mstar/gpio
EXTRA_CFLAGS      +=  -Idrivers/mstar/gpio/$(CONFIG_MSTAR_CHIP_NAME)
EXTRA_CFLAGS      +=  -Idrivers/mstar/include/$(CONFIG_MSTAR_CHIP_NAME)

# specific options
EXTRA_CFLAGS      +=  -DMSOS_TYPE_LINUX

# files
obj-$(CONFIG_MS_GPIO) += mdrv_gpio.o mdrv_gpio_io.o $(CONFIG_MSTAR_CHIP_NAME)/mhal_gpio.o
obj-$(CONFIG_MS_SW_I2C) += ms_gpioi2c.o mdrv_sw_iic.o
