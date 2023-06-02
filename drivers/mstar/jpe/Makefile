#
# Makefile for MStar Infinity JPE device drivers.

# general options
EXTRA_CFLAGS      +=  -Idrivers/mstar/jpe
EXTRA_CFLAGS      +=  -Idrivers/mstar/include

ccflags-$(CONFIG_MS_JPE)		+= -DMSOS_TYPE_LINUX_KERNEL

# files

#----------------------- drv -----------------------
EXTRA_CFLAGS      +=  -Idrivers/mstar/jpe/drv/jpe/inc
EXTRA_CFLAGS      +=  -Idrivers/mstar/jpe/drv/jpe/pub

#----------------------- hal -----------------------
EXTRA_CFLAGS      +=  -Idrivers/mstar/jpe/hal/jpe/inc
EXTRA_CFLAGS      +=  -Idrivers/mstar/jpe/hal/jpe/pub

#--------------------- sources ---------------------
obj-$(CONFIG_MS_JPE) += mstar_jpe.o
mstar_jpe-y := mdrv_jpe.o                       \
               jpe_scatter.o                    \
               hal/jpe/src/hal_jpe.o            \
               drv/jpe/src/drv_jpe.o
