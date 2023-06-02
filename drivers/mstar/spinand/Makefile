
obj-$(CONFIG_MS_SPINAND) += kdrv_spinand.o

# general options

#EXTRA_CFLAGS      +=  -Idrivers/mstar/include/infinity
EXTRA_CFLAGS      +=  -Idrivers/mstar/include
EXTRA_CFLAGS      +=  -Idrivers/mstar/spinand/hal
EXTRA_CFLAGS      +=  -Idrivers/mstar/spinand/drv/inc

# specific options
EXTRA_CFLAGS      +=  -DMSOS_TYPE_LINUX

kdrv_spinand-objs := drv/mdrv_spinand.o \
                     drv/mdrv_spinand_hal.o \
                        hal/mhal_spinand.o