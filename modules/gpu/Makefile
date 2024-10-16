#
# Copyright (C) 2015-2019 Allwinner Technology Limited. All rights reserved.
#
# This program is free software and is provided to you under the terms of the GNU
# General Public License version 2 as published by the Free Software Foundation, and
# any use by you of this program is subject to the terms of such GNU licence.
#
# Author: Albert Yu <yuxyun@allwinnertech.com>
#

ifneq ($(LICHEE_KDIR),)
export KERNEL_DIR = $(LICHEE_KDIR)
else
$(error LICHEE_KDIR is empty)
endif

export KDIR = $(KERNEL_DIR)
export KERNELDIR = $(KERNEL_DIR)

GPU_BUILD_TYPE = release

include $(KERNEL_DIR)/.config

ifdef CONFIG_OF
export USING_DT = 1
endif

##############################################################
##############     Set WINDOW_SYSTEM_TYPE      ###############
##############################################################
ifeq ($(LICHEE_PLATFORM), android)
OS_NAME = android
else ifeq ($(LICHEE_PLATFORM), secureandroid)
OS_NAME = android
else ifeq ($(LICHEE_PLATFORM), camdroid)
OS_NAME = android
else ifeq ($(LICHEE_PLATFORM), linux)
OS_NAME = linux
else ifeq ($(LICHEE_PLATFORM), dragonboard)
OS_NAME = linux
else
$(error LICHEE_PLATFORM is invalid)
endif

##############################################################
##################     Set GPU_TYPE         ##################
##############################################################
GPU_TYPE = $(CONFIG_SUNXI_GPU_TYPE)

ifneq ($(findstring $(GPU_TYPE), "mali400" "mali450"),)
GPU_ARCH := mali-utgard
else ifneq ($(findstring $(GPU_TYPE), "mali-t720" "mali-t760"),)
GPU_ARCH := mali-midgard
else ifneq ($(findstring $(GPU_TYPE), "mali-g31"),)
GPU_ARCH := mali-bifrost
else ifneq ($(findstring $(GPU_TYPE), "sgx544"),)
GPU_ARCH := img-sgx
else ifneq ($(findstring $(GPU_TYPE), "rgx6230" "ge8300"),)
GPU_ARCH := img-rgx
else
$(error GPU_TYPE($(GPU_TYPE)) is invalid)
endif

##############################################################
##########     Set GPU_BUILD_DIR and GPU_KO_NAME     #########
##############################################################
ifeq ($(CONFIG_ARM), y)
CPU_ARCH=arm
else ifeq ($(CONFIG_ARM64), y)
CPU_ARCH=arm64
else
$(error neither CONFIG_ARM nor CONFIG_ARM64 is found in $(KERNEL_DIR)/.config)
endif
ifeq ($(GPU_ARCH), mali-utgard)
GPU_BUILD_DIR = mali-utgard/driver/src/devicedrv/mali
GPU_KO_NAME   = $(GPU_BUILD_DIR)/mali.ko
else ifeq ($(GPU_ARCH), mali-midgard)
GPU_BUILD_DIR = mali-midgard/driver
GPU_KO_NAME   = $(GPU_BUILD_DIR)/drivers/gpu/arm/midgard/mali_kbase.ko
GPU_KO_NAME  += $(GPU_BUILD_DIR)/drivers/base/kds/kds.ko
GPU_KO_NAME  += $(GPU_BUILD_DIR)/drivers/base/dma_buf_test_exporter/dma-buf-test-exporter.ko
else ifeq ($(GPU_ARCH), mali-bifrost)
GPU_BUILD_DIR = mali-bifrost/driver
GPU_KO_NAME   = $(GPU_BUILD_DIR)/drivers/gpu/arm/midgard/mali_kbase.ko
GPU_KO_NAME  += $(GPU_BUILD_DIR)/drivers/base/dma_buf_test_exporter/dma-buf-test-exporter.ko
else ifeq ($(GPU_ARCH), img-sgx)
GPU_BUILD_DIR = img-sgx/$(OS_NAME)/eurasia_km/eurasiacon/build/linux2/sunxi_$(OS_NAME)
ifeq ($(CPU_ARCH), arm)
KO_DIR := img-sgx/$(OS_NAME)/eurasia_km/eurasiacon/binary2_sunxi_$(OS_NAME)_$(GPU_BUILD_TYPE)/target
else
$(error CPU_ARCH ($(CPU_ARCH) is invalid)
endif
GPU_KO_NAME   = $(KO_DIR)/pvrsrvkm.ko
ifeq ($(OS_NAME),linux)
GPU_KO_NAME   += $($(KO_DIR))/dc_sunxi.ko
endif
else ifeq ($(GPU_ARCH),img-rgx)
GPU_BUILD_DIR = img-rgx/$(OS_NAME)/rogue_km/build/linux/sunxi_$(OS_NAME)
ifeq ($(CPU_ARCH), arm)
KO_DIR := img-rgx/$(OS_NAME)/rogue_km/binary_sunxi_$(OS_NAME)_$(GPU_BUILD_TYPE)/target_armv7-a
else
#KO_DIR := img-rgx/$(OS_NAME)/rogue_km/binary_sunxi_$(OS_NAME)_$(GPU_BUILD_TYPE)/target_aarch64
KO_DIR := img-rgx/$(OS_NAME)/rogue_km/binary_sunxi_$(OS_NAME)_nullws_$(GPU_BUILD_TYPE)/target_aarch64
endif
GPU_KO_NAME   = $(KO_DIR)/pvrsrvkm.ko
ifeq ($(OS_NAME),linux)
GPU_KO_NAME   += $(KO_DIR)/dc_sunxi.ko
endif
else
$(error GPU_ARCH ($(GPU_ARCH)) is invalid)
endif

##############################################################

.PHONY: all build install clean

all: install

build:
	$(MAKE) -j -C $(GPU_BUILD_DIR) BUILD=$(GPU_BUILD_TYPE)

install: build
	@cp -uv $(GPU_KO_NAME) $(LICHEE_MOD_DIR);

clean:
	$(MAKE) -C $(GPU_BUILD_DIR) clean
