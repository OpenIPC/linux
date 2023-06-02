#
# Makefile for MStar fb device drivers.

# general options
EXTRA_CFLAGS                +=  -Idrivers/mstar/include
#--------gop--------
EXTRA_CFLAGS                +=  -Idrivers/mstar/fb/cedric/gop
EXTRA_CFLAGS                +=  -Idrivers/mstar/fb/cedric/gop/hal
#--------ge---------
EXTRA_CFLAGS                +=  -Idrivers/mstar/fb/cedric/ge
EXTRA_CFLAGS                +=  -Idrivers/mstar/fb/cedric/ge/columbus2

#EXTRA_CFLAGS                +=  -Idrivers/mstar/gfx

# specific options
EXTRA_CFLAGS                +=

# files
#obj-$(CONFIG_MS_FB_INFINITY)   += gop/mdrv_gop_io.o
#obj-$(CONFIG_MS_FB_INFINITY)   += gop/mdrv_gop.o
#obj-$(CONFIG_MS_FB_INFINITY)   += mdrv_fb.o

obj-$(CONFIG_MS_FB_INFINITY)   += infinity_fb.o

infinity_fb-objs := mdrv_fb.o \
                gop/mdrv_gop.o