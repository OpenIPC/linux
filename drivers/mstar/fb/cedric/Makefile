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
obj-$(CONFIG_MS_FB_CEDRIC)   += gop/mdrv_gop_io.o
obj-$(CONFIG_MS_FB_CEDRIC)   += gop/mdrv_gop.o
obj-$(CONFIG_MS_FB_CEDRIC)   += mdrv_fb.o
