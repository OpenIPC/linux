NVT_VENDOR_ISP_PATH=$(NVT_HDAL_DIR)/vendor/isp
NVT_VENDOR_DSP_PATH=$(NVT_HDAL_DIR)/vendor/dsp
NVT_VENDOR_CV_PATH=$(NVT_HDAL_DIR)/vendor/cv
NVT_VENDOR_AI_PATH=$(NVT_HDAL_DIR)/vendor/ai
NVT_VENDOR_AI2_PATH=$(NVT_HDAL_DIR)/vendor/ai2
NVT_VENDOR_MEDIA_PATH=$(NVT_HDAL_DIR)/vendor/media
NVT_VENDOR_THIRDPARTY_PATH=$(NVT_HDAL_DIR)/vendor/third_party
NVT_VENDOR_MAU_PATH=$(NVT_HDAL_DIR)/vendor/mau

all:
ifneq ($(wildcard $(NVT_VENDOR_ISP_PATH)/Makefile),)
	@$(MAKE) -C $(NVT_VENDOR_ISP_PATH)
endif
ifneq ($(wildcard $(NVT_VENDOR_DSP_PATH)/Makefile),)
	@$(MAKE) -C $(NVT_VENDOR_DSP_PATH)
endif
ifneq ($(wildcard $(NVT_VENDOR_CV_PATH)/Makefile),)
	@$(MAKE) -C $(NVT_VENDOR_CV_PATH)
endif
ifneq ($(wildcard $(NVT_VENDOR_AI_PATH)/Makefile),)
	@$(MAKE) -C $(NVT_VENDOR_AI_PATH)
endif
ifneq ($(wildcard $(NVT_VENDOR_AI2_PATH)/Makefile),)
	@$(MAKE) -C $(NVT_VENDOR_AI2_PATH)
endif
ifneq ($(wildcard $(NVT_VENDOR_MEDIA_PATH)/Makefile),)
	@$(MAKE) -C $(NVT_VENDOR_MEDIA_PATH)
endif
ifneq ($(wildcard $(NVT_VENDOR_THIRDPARTY_PATH)/Makefile),)
	@$(MAKE) -C $(NVT_VENDOR_THIRDPARTY_PATH)
endif
ifneq ($(wildcard $(NVT_VENDOR_MAU_PATH)/Makefile),)
	@$(MAKE) -C $(NVT_VENDOR_MAU_PATH)
endif

install:
ifneq ($(wildcard $(NVT_VENDOR_ISP_PATH)/Makefile),)
	@$(MAKE) -C $(NVT_VENDOR_ISP_PATH) install
endif
ifneq ($(wildcard $(NVT_VENDOR_DSP_PATH)/Makefile),)
	@$(MAKE) -C $(NVT_VENDOR_DSP_PATH) install
endif
ifneq ($(wildcard $(NVT_VENDOR_CV_PATH)/Makefile),)
	@$(MAKE) -C $(NVT_VENDOR_CV_PATH) install
endif
ifneq ($(wildcard $(NVT_VENDOR_AI_PATH)/Makefile),)
	@$(MAKE) -C $(NVT_VENDOR_AI_PATH) install
endif
ifneq ($(wildcard $(NVT_VENDOR_AI2_PATH)/Makefile),)
	@$(MAKE) -C $(NVT_VENDOR_AI2_PATH) install
endif
ifneq ($(wildcard $(NVT_VENDOR_MEDIA_PATH)/Makefile),)
	@$(MAKE) -C $(NVT_VENDOR_MEDIA_PATH) install
endif
ifneq ($(wildcard $(NVT_VENDOR_THIRDPARTY_PATH)/Makefile),)
	@$(MAKE) -C $(NVT_VENDOR_THIRDPARTY_PATH) install
endif
ifneq ($(wildcard $(NVT_VENDOR_MAU_PATH)/Makefile),)
	@$(MAKE) -C $(NVT_VENDOR_MAU_PATH) install
endif


clean:
ifneq ($(wildcard $(NVT_VENDOR_ISP_PATH)/Makefile),)
	@$(MAKE) -C $(NVT_VENDOR_ISP_PATH) clean
endif
ifneq ($(wildcard $(NVT_VENDOR_DSP_PATH)/Makefile),)
	@$(MAKE) -C $(NVT_VENDOR_DSP_PATH) clean
endif
ifneq ($(wildcard $(NVT_VENDOR_CV_PATH)/Makefile),)
	@$(MAKE) -C $(NVT_VENDOR_CV_PATH) clean
endif
ifneq ($(wildcard $(NVT_VENDOR_AI_PATH)/Makefile),)
	@$(MAKE) -C $(NVT_VENDOR_AI_PATH) clean
endif
ifneq ($(wildcard $(NVT_VENDOR_AI2_PATH)/Makefile),)
	@$(MAKE) -C $(NVT_VENDOR_AI2_PATH) clean
endif
ifneq ($(wildcard $(NVT_VENDOR_MEDIA_PATH)/Makefile),)
	@$(MAKE) -C $(NVT_VENDOR_MEDIA_PATH) clean
endif
ifneq ($(wildcard $(NVT_VENDOR_THIRDPARTY_PATH)/Makefile),)
	@$(MAKE) -C $(NVT_VENDOR_THIRDPARTY_PATH) clean
endif
ifneq ($(wildcard $(NVT_VENDOR_MAU_PATH)/Makefile),)
	@$(MAKE) -C $(NVT_VENDOR_MAU_PATH) clean
endif

codesize:
ifneq ($(wildcard $(NVT_VENDOR_ISP_PATH)/Makefile),)
	@$(MAKE) -C $(NVT_VENDOR_ISP_PATH) codesize
endif
ifneq ($(wildcard $(NVT_VENDOR_AI_PATH)/Makefile),)
	@$(MAKE) -C $(NVT_VENDOR_AI_PATH) codesize
endif
ifneq ($(wildcard $(NVT_VENDOR_MEDIA_PATH)/Makefile),)
	@$(MAKE) -C $(NVT_VENDOR_MEDIA_PATH) codesize
endif
ifneq ($(wildcard $(NVT_VENDOR_CV_PATH)/Makefile),)
	@$(MAKE) -C $(NVT_VENDOR_CV_PATH) codesize
endif