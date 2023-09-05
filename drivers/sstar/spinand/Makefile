CONFIG_SSTAR_CHIP_NAME := $(subst ",,$(CONFIG_SSTAR_CHIP_NAME))

obj-$(CONFIG_MS_SPINAND) += kdrv_spinand.o

# general options

#EXTRA_CFLAGS      +=  -Idrivers/sstar/include/$(CONFIG_SSTAR_CHIP_NAME)
EXTRA_CFLAGS      +=  -Idrivers/sstar/include
EXTRA_CFLAGS      +=  -Idrivers/sstar/spinand/hal/$(CONFIG_SSTAR_CHIP_NAME)
EXTRA_CFLAGS      +=  -Idrivers/sstar/spinand/drv/inc
EXTRA_CFLAGS      +=  -Idrivers/mtd

# specific options
EXTRA_CFLAGS      +=  -DMSOS_TYPE_LINUX

kdrv_spinand-objs := drv/mdrv_spinand.o \
                     drv/mdrv_spinand_hal.o \
                     hal/mhal_spinand.o