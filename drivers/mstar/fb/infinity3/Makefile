#
# Makefile for MStar fb device drivers.

# general options
EXTRA_CFLAGS                +=  -Idrivers/mstar/include
#--------gop--------
EXTRA_CFLAGS                +=  -Idrivers/mstar/fb/infinity3/gop
EXTRA_CFLAGS                +=  -Idrivers/mstar/fb/infinity3/inv_color

# files
#obj-$(CONFIG_MS_FB_INFINITY)   += gop/mdrv_gop_io.o
#obj-$(CONFIG_MS_FB_INFINITY)   += gop/mdrv_gop.o
#obj-$(CONFIG_MS_FB_INFINITY)   += mdrv_fb.o

obj-$(CONFIG_MS_FB_INFINITY3)   += infinity_fb.o

infinity_fb-objs := mdrv_fb.o \
                gop/mdrv_gop.o \
                inv_color/mdrv_gop0_inv_color.o