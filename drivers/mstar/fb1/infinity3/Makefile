#
# Makefile for MStar fb device drivers.

# general options
EXTRA_CFLAGS                +=  -Idrivers/mstar/include
#--------gop--------
EXTRA_CFLAGS                +=  -Idrivers/mstar/fb1/infinity3/gop1
EXTRA_CFLAGS                +=  -Idrivers/mstar/fb1/infinity3/inv_color

# files
#obj-$(CONFIG_MS_FB_INFINITY)   += gop/mdrv_gop_io.o
#obj-$(CONFIG_MS_FB_INFINITY)   += gop/mdrv_gop.o
#obj-$(CONFIG_MS_FB_INFINITY)   += mdrv_fb.o

obj-$(CONFIG_MS_FB1_INFINITY3)   += infinity_fb1.o

infinity_fb1-objs := mdrv_fb1.o \
                gop1/mdrv_gop1.o \
                inv_color/mdrv_gop1_inv_color.o