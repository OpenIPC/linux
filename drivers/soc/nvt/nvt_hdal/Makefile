#
# Makefile for the hdal built-in
#

VOS_DRIVER_SRC_DIR = vos/drivers/source
KDRV_SRC_DIR = hdal/drivers/k_driver/source

ifeq ($(CONFIG_NVT_VOS_KWRAP),y)
obj-$(CONFIG_NVT_VOS_KWRAP) += $(VOS_DRIVER_SRC_DIR)/kwrap/
endif

ifeq ($(CONFIG_NVT_HDAL_KDRV_BUILTIN),y)
obj-$(CONFIG_NVT_HDAL_KDRV_BUILTIN) += $(KDRV_SRC_DIR)/kdrv_builtin/
endif

