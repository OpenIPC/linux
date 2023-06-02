MAKE_PID := $(shell echo $$PPID)
JOB_FLAG := $(filter -j%, $(subst -j ,-j,$(shell ps T | grep "^\s*$(MAKE_PID).*$(MAKE)")))
JOBS     := $(subst -j,,$(JOB_FLAG))

NVT_VENDOR_AI_PATH=$(NVT_HDAL_DIR)/vendor/ai
NVT_KDRV_PATH=$(NVT_VENDOR_AI_PATH)/drivers/k_driver/source

all:
	@$(MAKE) -C $(NVT_KDRV_PATH) -j$(JOBS)

install:
	@$(MAKE) -C $(NVT_KDRV_PATH) modules_install

clean:
	@$(MAKE) -C $(NVT_KDRV_PATH) clean

codesize:
	@$(MAKE) -C $(NVT_KDRV_PATH) codesize
