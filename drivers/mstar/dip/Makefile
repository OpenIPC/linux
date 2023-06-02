#
# Makefile for MStar gop device drivers.

# general options
EXTRA_CFLAGS                +=  -Idrivers/mstar/include
EXTRA_CFLAGS                +=  -Idrivers/mstar/include/cedric

#-------------- msos --------------
EXTRA_CFLAGS                +=  -Idrivers/mstar/sca/drv/include
EXTRA_CFLAGS                +=  -Idrivers/mstar/sca/drv/msos
EXTRA_CFLAGS                +=  -Idrivers/mstar/sca/drv/drv/irq
EXTRA_CFLAGS                +=  -Idrivers/mstar/sca/drv/hal/c3/irq

#-------------- XC --------------
EXTRA_CFLAGS                +=  -Idrivers/mstar/sca/drv/api/xc
EXTRA_CFLAGS                +=  -Idrivers/mstar/sca/drv/drv/xc
EXTRA_CFLAGS                +=  -Idrivers/mstar/sca/drv/drv/xc/include
EXTRA_CFLAGS                +=  -Idrivers/mstar/sca/drv/hal/c3/xc
EXTRA_CFLAGS                +=  -Idrivers/mstar/sca/drv/hal/c3/xc/include

# specific options
EXTRA_CFLAGS                +=


ccflags-y		+= -DMSOS_TYPE_LINUX_KERNEL
ccflags-y		+= -DCHIP_C3
ccflags-y		+= -DMULTI_SCALER_SUPPORTED


# files
obj-$(CONFIG_MS_DIP)   += mdrv_dip_io.o
