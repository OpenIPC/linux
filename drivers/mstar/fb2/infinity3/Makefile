#
# Makefile for MStar fb device drivers.

# general options
EXTRA_CFLAGS                +=  -Idrivers/mstar/include
#--------gop--------
EXTRA_CFLAGS                +=  -Idrivers/mstar/fb2/infinity3/gop2
EXTRA_CFLAGS                +=  -Idrivers/mstar/fb2/infinity3/inv_color

# files
#obj-$(CONFIG_MS_FB_INFINITY)   += gop/mdrv_gop_io.o
#obj-$(CONFIG_MS_FB_INFINITY)   += gop/mdrv_gop.o
#obj-$(CONFIG_MS_FB_INFINITY)   += mdrv_fb.o

obj-$(CONFIG_MS_FB2_INFINITY3)   += infinity_fb2.o

infinity_fb2-objs := mdrv_fb2.o \
                gop2/mdrv_gop2.o \
                inv_color/mdrv_gop2_inv_color.o