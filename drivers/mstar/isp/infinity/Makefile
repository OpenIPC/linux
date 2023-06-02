#
# Makefile for MStar ISP drivers.
#


# general options
EXTRA_CFLAGS += -Idrivers/mstar/include
EXTRA_CFLAGS += -Iarch/arm/include
# # MsTypes.h
EXTRA_CFLAGS += -Iinclude/linux

# files
obj-$(CONFIG_MS_ISP_INFINITY)  += hal_isp.o drv_isp.o i2c.o
