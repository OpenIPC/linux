#
# Makefile for Camdriver dip device drivers.
MS_ARCH_NAME := $(subst ",,$(CONFIG_SSTAR_CHIP_NAME))
#----------------------- drv -----------------------
EXTRA_CFLAGS                +=  -Idrivers/sstar/camclk
EXTRA_CFLAGS                +=  -Idrivers/sstar/camclk/drv/inc
EXTRA_CFLAGS                +=  -Idrivers/sstar/camclk/drv/pub
#----------------------- hal -----------------------
EXTRA_CFLAGS                +=  -Idrivers/sstar/camclk/hal/common
EXTRA_CFLAGS                +=  -Idrivers/sstar/camclk/hal/$(MS_ARCH_NAME)/inc
# general options
EXTRA_CFLAGS                +=  -Idrivers/sstar/include
EXTRA_CFLAGS                +=  -Idrivers/sstar/include/$(MS_ARCH_NAME)/
# specific options
EXTRA_CFLAGS                +=

ccflags-y		+= -Dcamclk_IO_OFFSET -DCONFIG_NOTTOGATED
ifdef CONFIG_CAM_CLK_SYSFS
ccflags-y		+= -DCAM_CLK_SYSFS
endif
#--------------------- sources ---------------------
obj-$(CONFIG_CAM_CLK)   += camclk.o
camclk-y :=             \
                hal/$(MS_ARCH_NAME)/src/hal_camclk_complex.o      \
                hal/$(MS_ARCH_NAME)/src/hal_camclk_tbl.o      \
                drv/src/linux/drv_camclk_module.o\
                drv/src/linux/drv_camclk_sysfs.o\
                drv/src/linux/drv_camclk_os.o\
                drv/src/drv_camclk_impl.o\
                drv/src/drv_camclk.o\
                drv/src/drv_camclk_debug.o
