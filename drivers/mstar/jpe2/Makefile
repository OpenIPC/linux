#
# Makefile for MStar Infinity JPE device drivers.

# general options
EXTRA_CFLAGS      +=  -Idrivers/mstar/cam_os_wrapper/pub
EXTRA_CFLAGS      +=  -Idrivers/mstar/jpe2
EXTRA_CFLAGS      +=  -Idrivers/mstar/include

ccflags-$(CONFIG_MS_JPE)		+= -DMSOS_TYPE_LINUX_KERNEL

# files

#----------------------- drv -----------------------
EXTRA_CFLAGS      +=  -Idrivers/mstar/jpe2/drv/inc
EXTRA_CFLAGS      +=  -Idrivers/mstar/jpe2/drv/pub
EXTRA_CFLAGS      +=  -Idrivers/mstar/jpe2/

#----------------------- hal -----------------------
EXTRA_CFLAGS      +=  -Idrivers/mstar/jpe2/hal/pub

#--------------------- sources ---------------------
obj-$(CONFIG_MS_JPE) += mstar_jpe.o
mstar_jpe-y :=  hal/src/hal_jpe_ios.o           \
                hal/src/hal_jpe_ops.o           \
                drv/src/common/drv_jpe_ctx.o    \
                drv/src/common/drv_jpe_dev.o    \
                drv/src/linux/drv_jpe_module.o  \
                ../cam_os_wrapper/src/cam_os_wrapper.o
# drv/src/common/cam_os_wrapper.o