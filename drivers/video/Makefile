obj-$(CONFIG_VGASTATE)       += vgastate.o
obj-$(CONFIG_HDMI)           += hdmi.o

obj-$(CONFIG_VT)		     += console/
obj-$(CONFIG_LOGO)		     += logo/
obj-y				         += backlight/
obj-y				         += fbdev/

obj-$(CONFIG_JZ_I2D)		 += ingenic_i2d/
obj-$(CONFIG_JZ_IPU)         += ingenic_ipu/
obj-$(CONFIG_JZ_BSCALER)          += ingenic_bscaler/

obj-$(CONFIG_VIDEOMODE_HELPERS) += display_timing.o videomode.o
ifeq ($(CONFIG_OF),y)
obj-$(CONFIG_VIDEOMODE_HELPERS) += of_display_timing.o of_videomode.o
endif
