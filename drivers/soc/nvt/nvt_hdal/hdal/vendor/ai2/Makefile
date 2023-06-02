MAKE_PID := $(shell echo $$PPID)
JOB_FLAG := $(filter -j%, $(subst -j ,-j,$(shell ps T | grep "^\s*$(MAKE_PID).*$(MAKE)")))
JOBS     := $(subst -j,,$(JOB_FLAG))

NVT_VENDOR_AI2_PATH=$(NVT_HDAL_DIR)/vendor/ai2
NVT_KFLOW_PATH=$(NVT_VENDOR_AI2_PATH)/drivers/k_flow/source
NVT_SOURCE_PATH=$(NVT_VENDOR_AI2_PATH)/source
NVT_SOURCE_PUB_PATH=$(NVT_VENDOR_AI2_PATH)/source_pub
#NVT_SAMPLE_PATH=$(NVT_VENDOR_AI2_PATH)/samples

all:
	@$(MAKE) -C $(NVT_KFLOW_PATH) -j$(JOBS)
	@$(MAKE) -C $(NVT_SOURCE_PATH) -j$(JOBS)
	@$(MAKE) -C $(NVT_SOURCE_PUB_PATH) -j$(JOBS)
	#@$(MAKE) -C $(NVT_SAMPLE_PATH) -j$(JOBS)

install:
	@$(MAKE) -C $(NVT_KFLOW_PATH) modules_install
	@$(MAKE) -C $(NVT_SOURCE_PATH) install
	@$(MAKE) -C $(NVT_SOURCE_PUB_PATH) install

clean:
	@$(MAKE) -C $(NVT_KFLOW_PATH) clean
	@$(MAKE) -C $(NVT_SOURCE_PATH) clean
	@$(MAKE) -C $(NVT_SOURCE_PUB_PATH) clean
	#@$(MAKE) -C $(NVT_SAMPLE_PATH) clean

codesize:
	@$(MAKE) -C $(NVT_KFLOW_PATH) codesize
	#@$(MAKE) -C $(NVT_SOURCE_PATH) codesize
	#@$(MAKE) -C $(NVT_SOURCE_PUB_PATH) codesize
