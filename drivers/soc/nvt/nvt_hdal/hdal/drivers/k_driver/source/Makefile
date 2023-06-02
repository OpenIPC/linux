GROUP_KO=ON
VOS_DRIVER_DIR = $(NVT_VOS_DIR)/drivers
KDRV_DIR=$(NVT_HDAL_DIR)/drivers/k_driver
SUBDIRS := $(dir $(shell find . -name Makefile))
SUBDIRS := $(shell echo $(SUBDIRS) | sed -e 's/\.\///g';)
# The current directory is passed to sub-makes as argument
PWD := $(shell pwd)
.PHONY: modules modules_install clean $(SUBDIRS)
###############################################################################
# Linux Makefile                                                              #
###############################################################################
ifeq ($(NVT_PRJCFG_CFG),Linux)
# To build modules outside of the kernel tree, we run "make"
# in the kernel source tree; the Makefile these then includes this
# Makefile once again.
# This conditional selects whether we are being included from the
# kernel Makefile or not.
ifeq ($(KERNELRELEASE),)
export KBUILD_EXTRA_SYMBOLS = $(shell find $(VOS_DRIVER_DIR) -name Module.symvers)

modules:
	@$(MAKE) -C $(KERNELDIR) M=$(PWD) $(NVT_KGCOV) modules $(NVT_MULTI_CORES_FLAG);

modules_install:
	@if [ -z $(NVT_MOD_INSTALL) ]; then \
		$(MAKE) -C $(KERNELDIR) M=$(PWD) INSTALL_MOD_PATH=./_install_modules/ INSTALL_MOD_DIR=hdal modules_install; \
	else \
		$(MAKE) -C $(KERNELDIR) M=$(PWD) INSTALL_MOD_PATH=$(NVT_MOD_INSTALL) INSTALL_MOD_DIR=hdal modules_install; \
	fi

clean:
	@rm -rf Module.symvers modules.order .tmp_versions
	@for n in $(SUBDIRS); do \
		$(MAKE) -C $$n clean $(NVT_MULTI_CORES_FLAG); \
	done

codesize:
	@for n in $(SUBDIRS); do \
		$(MAKE) -C $$n codesize $(NVT_MULTI_CORES_FLAG); \
	done
else
# called from kernel build system: just declare what our modules are
obj-m += \
	comm/dummy/ \
	comm/nvtmem/ \
	comm/usb_chrgdet/ \
	comm/usb2dev/ \
	comm/drvdump/ \
	comm/tse/ \
	comm/remote/ \
	comm/uvcp/ \
	kdrv_videoout/ide/ \
	kdrv_videoout/display_obj/ \
	kdrv_videoout/dsi/ \
	kdrv_videoout/fbdev/ \
	kdrv_videocodec/ \
	kdrv_gfx2d/kdrv_affine/ \
	kdrv_gfx2d/kdrv_affine/affine_neon/ \
	kdrv_audioio/audio_common/ \
	kdrv_audioio/audio_device/ac108/ \
	kdrv_audioio/audio_device/emu/ \
	kdrv_audioio/kdrv_audio/ \
	kdrv_audioio/audlib_aec/ \
	kdrv_audioio/audlib_agc/ \
	kdrv_audioio/audlib_anr/ \
	kdrv_audioio/audlib_filt/ \
	kdrv_audioio/audlib_src/ \
	kdrv_audioio/audlib_adpcm/ \
	kdrv_audioio/audlib_g711/ \
	kdrv_audioio/audlib_aac/ \
	#kdrv_videoprocess/kdrv_ipp_sim/ \

ifeq ($(GROUP_KO),ON)
obj-m += \
	kdrv_builtin/ \
	comm/ \
	kdrv_videocapture/ \
	kdrv_videoprocess/ \
	kdrv_gfx2d/ \

else
obj-m += \
	kdrv_builtin/ \
	comm/kdrv_log/ \
	comm/drv_sys/ \
	comm/sif/ \
	comm/pwm/ \
	comm/ddr_arb/ \
	kdrv_gfx2d/kdrv_grph/ \
	kdrv_gfx2d/kdrv_ise/ \
	kdrv_videocapture/kdrv_sie/ \
	kdrv_videocapture/kdrv_tge/ \
	kdrv_videocapture/ssenif/senphy/ \
	kdrv_videocapture/ssenif/lvds/ \
	kdrv_videocapture/ssenif/csi/ \
	kdrv_videocapture/ssenif/hal/ \
	kdrv_videoprocess/kdrv_ife/ \
	kdrv_videoprocess/kdrv_ipe/ \
	kdrv_videoprocess/kdrv_dce/ \
	kdrv_videoprocess/kdrv_ife2/ \
	kdrv_videoprocess/kdrv_ime/ \

endif
endif

###############################################################################
# rtos Makefile                                                               #
###############################################################################
else ifeq ($(NVT_PRJCFG_CFG),rtos)

FILTER_OUT = $(foreach v,$(2),$(if $(findstring $(1),$(v)),,$(v)))
ifeq ("${RTOS_CPU_TYPE}","cortex-a53")
SUBDIRS := $(call FILTER_OUT,rtos_na51089,$(SUBDIRS))
else ifeq ("${RTOS_CPU_TYPE}","cortex-a9")
SUBDIRS := $(call FILTER_OUT,rtos_na51000,$(SUBDIRS))
endif

ifeq ("${OLD_IPP}","ON")
SUBDIRS := $(call FILTER_OUT,kdrv_videoprocess,$(SUBDIRS))
else
SUBDIRS := $(call FILTER_OUT,emu_kdrv_vproc,$(SUBDIRS))
endif

modules: $(SUBDIRS)

$(SUBDIRS):
	@$(MAKE) -C $@  modules $(NVT_MULTI_CORES_FLAG)

modules_install:
	@for n in $(SUBDIRS); do \
		$(MAKE) -C $$n modules_install; \
	done

clean:
	@for n in $(SUBDIRS); do \
		$(MAKE) -C $$n clean; \
	done
	@rm ../output -rf
endif
