#
# Makefile for the davinci video device drivers.
#

# VPIF
obj-$(CONFIG_VIDEO_DAVINCI_VPIF) += vpif.o

#DM646x EVM Display driver
obj-$(CONFIG_DISPLAY_DAVINCI_DM646X_EVM) += vpif_display.o
#DM646x EVM Capture driver
obj-$(CONFIG_CAPTURE_DAVINCI_DM646X_EVM) += vpif_capture.o

# Capture: DM6446, DM355, DM365
obj-$(CONFIG_VIDEO_VPSS_SYSTEM) += vpss.o
obj-$(CONFIG_VIDEO_VPFE_CAPTURE) += vpfe_capture.o
obj-$(CONFIG_VIDEO_DM6446_CCDC) += dm644x_ccdc.o
obj-$(CONFIG_VIDEO_DM355_CCDC) += dm355_ccdc.o
obj-$(CONFIG_VIDEO_DM365_ISIF) += dm365_ccdc.o
dm355_af_driver-objs		+= dm355_af.o dm355_af_hw.o
obj-$(CONFIG_VIDEO_DM355_AF)	+= dm355_af_driver.o
dm355_aew_driver-objs		+= dm355_aew.o dm355_aew_hw.o
obj-$(CONFIG_VIDEO_DM355_AEW)	+= dm355_aew_driver.o

dm365_a3_hw_driver-objs := dm365_a3_hw.o
obj-$(CONFIG_VIDEO_DM365_3A_HW)	+= dm365_a3_hw_driver.o
dm365_af_driver-objs := dm365_af.o
obj-$(CONFIG_VIDEO_DM365_AF)	+= dm365_af_driver.o
dm365_aew_driver-objs := dm365_aew.o
obj-$(CONFIG_VIDEO_DM365_AEW)	+= dm365_aew_driver.o

# DaVinci Encoder/Display Manager and Encoders
obj-$(CONFIG_DAVINCI_OSD)		+= davinci_platform.o davinci_osd.o
obj-$(CONFIG_DAVINCI_ENC_MNGR)		+= davinci_enc_mngr.o

obj-$(CONFIG_DAVINCI_LOGICPD_ENCODER)	+= logicpd_encoder.o
obj-$(CONFIG_DAVINCI_AVNET_ENCODER)	+= avnetlcd_encoder.o
obj-$(CONFIG_DAVINCI_VPBE_ENCODER)	+= vpbe_encoder.o
obj-$(CONFIG_DAVINCI_GENERIC_PRGB_ENCODER)	+= dm365_generic_prgb_encoder.o

obj-$(CONFIG_VIDEO_DAVINCI_DISPLAY)	+= davinci_display.o
