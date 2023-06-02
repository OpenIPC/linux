#
# Makefile for MStar fb device drivers.

# general options
EXTRA_CFLAGS                +=  -Idrivers/mstar/include
#--------gop--------
EXTRA_CFLAGS                +=  -Idrivers/mstar/fb/chicago/gop
EXTRA_CFLAGS                +=  -Idrivers/mstar/fb/chicago/gop/hal
#--------ge---------
EXTRA_CFLAGS                +=  -Idrivers/mstar/fb/chicago/ge
EXTRA_CFLAGS                +=  -Idrivers/mstar/fb/chicago/ge/columbus2

#EXTRA_CFLAGS                +=  -Idrivers/mstar/gfx

# specific options
EXTRA_CFLAGS                +=

# files
obj-$(CONFIG_MS_FB_CHICAGO)   += gop/mdrv_gop_io.o
obj-$(CONFIG_MS_FB_CHICAGO)   += gop/mdrv_gop.o
obj-$(CONFIG_MS_FB_CHICAGO)   += mdrv_fb.o
