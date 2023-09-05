#
# Makefile for MStar Infinity3 IVE device drivers.

# general options
EXTRA_CFLAGS      +=  -Idrivers/sstar/ive
EXTRA_CFLAGS      +=  -Idrivers/sstar/include

ccflags-$(CONFIG_MS_IVE)		+= -DMSOS_TYPE_LINUX_KERNEL

# files
obj-$(CONFIG_MS_IVE) += mstar_ive.o
mstar_ive-y := mdrv_ive.o   \
               hal_ive.o    \
               drv_ive.o    \
			   hal_ive_simulate.o
