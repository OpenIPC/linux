#
# Makefile for the linux kernel.
#

ccflags-$(CONFIG_ARCH_NVT_IVOT_V7) := -I$(srctree)/arch/arm/plat-novatek/include

obj-$(CONFIG_NVT_IVOT_PLAT_NA51089)         += plat-na51089/reset.o plat-na51089/pll_preset.o plat-na51089/fastboot_preload.o plat-na51089/outstanding_control.o nvt-sramctl.o
obj-$(CONFIG_NVT_IVOT_PLAT_NA51068)         += plat-na51068/reset.o plat-na51068/nvt_jiffies.o plat-na51068/pll_preset.o nvt-sramctl.o
obj-$(CONFIG_NVT_IVOT_PLAT_NA51055)         += plat-na51055/reset.o plat-na51055/pll_preset.o plat-na51055/fastboot_preload.o plat-na51055/outstanding_control.o nvt-sramctl.o

nvt-platform-$(CONFIG_NVT_IVOT_PLAT_NA51089)		:= plat-na51089
nvt-platform-$(CONFIG_NVT_IVOT_PLAT_NA51068)		:= plat-na51068
nvt-platform-$(CONFIG_NVT_IVOT_PLAT_NA51055)		:= plat-na51055

PLATFORM_DIR   := $(nvt-platform-y)

define create-platform-symlink
  MYKERNEL_PATH=$(shell cd $(srctree); pwd);
endef

arch/arm/plat-novatek/include/plat: FORCE
	$(Q) $(create-platform-symlink)

prepare: arch/arm/plat-novatek/include/plat
