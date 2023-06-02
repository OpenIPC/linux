#
# Makefile for MStar IIC device drivers.
#

CONFIG_MSTAR_CHIP_NAME := $(subst ",,$(CONFIG_MSTAR_CHIP_NAME))
# general options
# EXTRA_CFLAGS      +=  -Idrivers/mstar/common
EXTRA_CFLAGS      +=  -Idrivers/mstar/include
EXTRA_CFLAGS      +=  -Idrivers/mstar/i2c
EXTRA_CFLAGS      +=  -Idrivers/mstar/i2c/$(CONFIG_MSTAR_CHIP_NAME)


# specific options
EXTRA_CFLAGS      +=  -DTITANIA
EXTRA_CFLAGS      +=  -DMSOS_TYPE_LINUX

# files
obj-$(CONFIG_MS_I2C_CEDRIC) += ms_iic.o $(CONFIG_MSTAR_CHIP_NAME)/mhal_iic.o
obj-$(CONFIG_MS_I2C_INFINITY) +=ms_iic.o $(CONFIG_MSTAR_CHIP_NAME)/mhal_iic.o
obj-$(CONFIG_MS_I2C_INFINITY3) +=ms_iic.o $(CONFIG_MSTAR_CHIP_NAME)/mhal_iic.o

# export header files
EXPORT_H_FILES    += mdrv_iic_io.h
