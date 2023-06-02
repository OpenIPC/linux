NVT_KDRV_PATH=$(NVT_HDAL_DIR)/drivers/k_driver/source
NVT_KFLOW_PATH=$(NVT_HDAL_DIR)/drivers/k_flow/source
NVT_SOURCE_PATH=$(NVT_HDAL_DIR)/source
NVT_SAMPLES_PATH=$(NVT_HDAL_DIR)/samples
NVT_TEST_PATH=$(NVT_HDAL_DIR)/test_cases
NVT_EXT_DEV_PATH=$(NVT_HDAL_DIR)/ext_devices
NVT_VENDOR_PATH=$(NVT_HDAL_DIR)/vendor

all:
ifneq ($(wildcard $(NVT_KDRV_PATH)/Makefile),)
	@$(MAKE) -C $(NVT_KDRV_PATH)
endif
ifneq ($(wildcard $(NVT_KFLOW_PATH)/Makefile),)
	@$(MAKE) -C $(NVT_KFLOW_PATH)
endif
ifneq ($(wildcard $(NVT_SOURCE_PATH)/Makefile),)
	@$(MAKE) -C $(NVT_SOURCE_PATH) $(NVT_MULTI_CORES_FLAG)
endif
ifneq ($(wildcard $(NVT_VENDOR_PATH)/Makefile),)
	@$(MAKE) -C $(NVT_VENDOR_PATH)
endif
ifneq ($(wildcard $(NVT_EXT_DEV_PATH)/Makefile),)
	@$(MAKE) -C $(NVT_EXT_DEV_PATH)
endif

install:
ifneq ($(wildcard $(NVT_KDRV_PATH)/Makefile),)
	@$(MAKE) -C $(NVT_KDRV_PATH) modules_install
endif
ifneq ($(wildcard $(NVT_KFLOW_PATH)/Makefile),)
	@$(MAKE) -C $(NVT_KFLOW_PATH) modules_install
endif
ifneq ($(wildcard $(NVT_SOURCE_PATH)/Makefile),)
	@$(MAKE) -C $(NVT_SOURCE_PATH) install
endif
ifneq ($(wildcard $(NVT_VENDOR_PATH)/Makefile),)
	@$(MAKE) -C $(NVT_VENDOR_PATH) install
endif
ifneq ($(wildcard $(NVT_EXT_DEV_PATH)/Makefile),)
	@$(MAKE) -C $(NVT_EXT_DEV_PATH) modules_install
endif

sample:
ifneq ($(wildcard $(NVT_SAMPLES_PATH)/Makefile),)
	@$(MAKE) -C $(NVT_SAMPLES_PATH)
endif

sample_install:
ifneq ($(wildcard $(NVT_SAMPLES_PATH)/Makefile),)
	@$(MAKE) -C $(NVT_SAMPLES_PATH) install
endif

test:
ifneq ($(wildcard $(NVT_TEST_PATH)/Makefile),)
	@$(MAKE) -C $(NVT_TEST_PATH)
endif

test_install:
ifneq ($(wildcard $(NVT_TEST_PATH)/Makefile),)
	@$(MAKE) -C $(NVT_TEST_PATH) install
endif

clean:
ifneq ($(wildcard $(NVT_KDRV_PATH)/Makefile),)
	@$(MAKE) -C $(NVT_KDRV_PATH) clean
endif
ifneq ($(wildcard $(NVT_KFLOW_PATH)/Makefile),)
	@$(MAKE) -C $(NVT_KFLOW_PATH) clean
endif
ifneq ($(wildcard $(NVT_SOURCE_PATH)/Makefile),)
	@$(MAKE) -C $(NVT_SOURCE_PATH) clean
endif
ifneq ($(wildcard $(NVT_SAMPLES_PATH)/Makefile),)
	@$(MAKE) -C $(NVT_SAMPLES_PATH) clean
endif
ifneq ($(wildcard $(NVT_TEST_PATH)/Makefile),)
	@$(MAKE) -C $(NVT_TEST_PATH) clean
endif
ifneq ($(wildcard $(NVT_VENDOR_PATH)/Makefile),)
	@$(MAKE) -C $(NVT_VENDOR_PATH) clean
endif
ifneq ($(wildcard $(NVT_EXT_DEV_PATH)/Makefile),)
	@$(MAKE) -C $(NVT_EXT_DEV_PATH) clean
endif
codesize:
ifneq ($(wildcard $(NVT_VENDOR_PATH)/Makefile),)
	@$(MAKE) -C $(NVT_VENDOR_PATH) codesize
endif
ifneq ($(wildcard $(NVT_KDRV_PATH)/Makefile),)
	@$(MAKE) -C $(NVT_KDRV_PATH) codesize
endif
ifneq ($(wildcard $(NVT_KFLOW_PATH)/Makefile),)
	@$(MAKE) -C $(NVT_KFLOW_PATH) codesize
endif