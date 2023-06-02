MODULE_NAME = nvt_fb
# DIRs
VOS_DRIVER_DIR = $(NVT_VOS_DIR)/drivers
KDRV_DIR = $(NVT_HDAL_DIR)/drivers/k_driver
CURR_DRIVER_DIR = $(KDRV_DIR)
RTOS_OUTPUT_DIR = $(CURR_DRIVER_DIR)/output
# INCs
VOS_INC_PATH = $(VOS_DRIVER_DIR)/include
KDRV_INC_PATH = $(KDRV_DIR)/include
CURR_DRIVER_INC_PATH = $(CURR_DRIVER_DIR)/include
# INC FLAGs

EXTRA_INCLUDE = -I$(VOS_INC_PATH) -I$(KDRV_INC_PATH) -I$(CURR_DRIVER_INC_PATH) -I$(KDRV_DIR)/source/kdrv_videoout/include/

ifeq ("${RTOS_CPU_TYPE}","cortex-a53")
EXTRA_INCLUDE += -I$(KDRV_INC_PATH)/rtos_na51000
else
EXTRA_INCLUDE += -I$(KDRV_INC_PATH)/rtos_na51089
endif

.PHONY: modules modules_install clean

ifeq ($(NVT_PRJCFG_CFG),Linux)

ifeq ($(CONFIG_NVT_FB_DEBUG), y)
EXTRA_CFLAGS += -DFB_NA51000_DEBUG
endif

EXTRA_CFLAGS += $(EXTRA_INCLUDE) -D__LINUX

obj-m          += $(MODULE_NAME).o
OBJ := nvt_fb_na51000.o nvt_fb_osd.o nvt_fb_video.o
ccflags-y += -I$(NVT_DRIVER_DIR)/include
ccflags-y += -I$(VOS_INC_PATH)
ccflags-y += -I$(NVT_HDAL_DIR)/drivers/k_driver/source/kdrv_videoout/include
ccflags-y += -I$(NVT_HDAL_DIR)/drivers/k_driver/include/
ccflags-y += -Werror

ifeq ($(CONFIG_FUNCTION_TRACER),y)
$(MODULE_NAME)-objs := $(OBJ)
else
$(MODULE_NAME)-objs := $(OBJ:.o=_no_tracer.o)
EXTRA_CFLAGS += $(shell for n in $(OBJ:.o=); do if [ -f $(src)/$$n.c ]; then ln -sf $$(basename $${n}.c) $(src)/$${n}_no_tracer.c; else rm -f $(src)/$${n}_no_tracer.c; fi done)
endif

ifeq ($(KERNELRELEASE),)
PWD := $(shell pwd)
KERVER ?= $(NVT_LINUX_VER)
KDIR ?= $(KERNELDIR)
MDIR ?= $(KERNELDIR)/_install_modules/lib/modules/$(KERVER)/extra
MODPATH := $(shell echo $(PWD) | awk -F'linux-driver/' '{print $$NF}')
MODNAME := $(shell echo $(obj-m:.o=.ko))
# variables for clean object
RM_TRACER_OBJ = $(foreach n, $(OBJ:.o=), $(if $(wildcard $(n).c),$(if $(wildcard $(n).o),$(n).o,),))
RM_NO_TRACER_OBJ = $(foreach n, $(OBJ:.o=), $(if $(wildcard $(n).c),$(if $(wildcard $(n)_no_tracer.o),$(n)_no_tracer.o,),))
KBUILD_EXTRA_SYMBOLS += $(shell find $(NVT_HDAL_DIR) -name Module.symvers)

modules:
	echo $(ccflags-y)
	echo $(KBUILD_EXTRA_SYMBOLS)
	$(MAKE) -C $(KDIR) M=$(PWD) $(NVT_KGCOV) modules

modules_install:
	@if [ -z $(NVT_MOD_INSTALL) ]; then \
		rm -f $(MDIR)/$(MODPATH)/$(MODNAME); \
		install -m644 -b -D $(MODNAME) ${MDIR}/$(MODPATH)/$(MODNAME); \
		cd $(KDIR)/_install_modules/lib/modules/$(KERVER)/; depmod -b $(KDIR)/_install_modules/ -a $(KERVER); \
		cd $(KDIR)/_install_modules/lib/modules/$(KERVER)/;\
	else \
		mkdir -p $(NVT_MOD_INSTALL)/lib/modules/$(KERVER); \
		install -m644 -b -D $(MODNAME) $(NVT_MOD_INSTALL)/lib/modules/$(KERVER)/extra/$(MODPATH)/$(MODNAME); \
	fi

clean:
	@rm -rf $(MODULE_NAME).* .$(MODULE_NAME).* $(OBJ:.o=_no_tracer.c) Module.symvers modules.order .tmp_versions
	@rm -f `find . -type f -name ".*.cmd" -o -name ".*.d"`
	@rm -f $(RM_TRACER_OBJ) $(RM_NO_TRACER_OBJ)

codesize:
	@echo $(MODULE_NAME)
	@$(OBJDUMP) -t $(MODULE_NAME).o > $(MODULE_NAME).sym && \
	$(BUILD_DIR)/nvt-tools/nvt-ld-op -j $(MODULE_NAME).sym

endif

else ifeq ($(NVT_PRJCFG_CFG),rtos)
modules:
	@echo "nothing to be done for '$(MODULE_NAME)'"

clean:
	@echo "nothing to be done for '$(MODULE_NAME)'"

modules_install:
	@echo "nothing to be done for '$(MODULE_NAME)'"
endif
