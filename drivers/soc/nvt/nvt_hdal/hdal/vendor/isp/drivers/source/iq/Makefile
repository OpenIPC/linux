MODULE_NAME = nvt_iq
# DIRs
VOS_DRIVER_DIR = $(NVT_VOS_DIR)/drivers
KDRV_DIR = $(NVT_HDAL_DIR)/drivers/k_driver
KFLOW_DIR = $(NVT_HDAL_DIR)/drivers/k_flow
CURR_DRIVER_DIR = $(NVT_HDAL_DIR)/vendor/isp/drivers
RTOS_OUTPUT_DIR = $(CURR_DRIVER_DIR)/output
PREBUILT_DIR = $(NVT_HDAL_DIR)/vendor/isp/drivers/source/iq
# INCs
VOS_INC_PATH = $(VOS_DRIVER_DIR)/include
KDRV_INC_PATH = $(KDRV_DIR)/include
KFLOW_INC_PATH = $(KFLOW_DIR)/include
CURR_DRIVER_INC_PATH = $(CURR_DRIVER_DIR)/include
ALG_ISP_DRV_INC_PATH=$(NVT_HDAL_DIR)/vendor/alg/isp/drivers
# INC FLAGs

EXTRA_INCLUDE += \
	-I$(VOS_INC_PATH) \
	-I$(KDRV_INC_PATH) \
	-I$(KDRV_INC_PATH)/comm \
	-I$(KFLOW_INC_PATH) \
	-I$(ALG_ISP_DRV_INC_PATH)/include/ \
	-I$(CURR_DRIVER_INC_PATH) \
	-I$(ALG_ISP_DRV_INC_PATH)/source/iq/ \
	-I$(ALG_ISP_DRV_INC_PATH)/source/iq/iq_lib/ \
	-I$(ALG_ISP_DRV_INC_PATH)/source/iq/iq_param/ \
	-I$(ALG_ISP_DRV_INC_PATH)/source/iq/iq_task/ \
	-I$(ALG_ISP_DRV_INC_PATH)/source/iq/iq_ui/

.PHONY: modules modules_install clean
###############################################################################
# Linux Makefile                                                              #
###############################################################################
ifeq ($(NVT_PRJCFG_CFG),Linux)
# to solve the 'make modules_install' got empty CONFIG_OLD_MCOUNT
ifeq ($(KERNELRELEASE),)
-include $(KERNELDIR)/.config
endif

C_CFLAGS  += -D__LINUX -Werror

EXTRA_CFLAGS += $(C_CFLAGS) $(EXTRA_INCLUDE) -Wno-date-time  -I$(src)/include -DDEBUG
KBUILD_EXTRA_SYMBOLS = $(shell find $(VOS_DRIVER_DIR) -name Module.symvers) $(shell find $(KDRV_DIR) -name Module.symvers) $(shell find $(KFLOW_DIR) -name Module.symvers) $(shell find $(CURR_DRIVER_DIR) -name Module.symvers)

obj-m += $(MODULE_NAME).o

OBJ := \
	iq_dbg.o \
	iq_main.o \
	iq_proc.o \
	iq_version.o \
	iq_lib/iq_alg.o \
	iq_lib/iq_lib.o \
	iq_lib/iq_msg.o \
	iq_lib/iq_common_param.o \
	iq_lib/iq_flow.o \
	iq_lib/iq_flow_ext.o \
	iq_lib/iqt_api.o \
	iq_lib/iqt_cfg.o \
	iq_lib/iqt_dtsi.o \
	iq_param/iq_param_default.o \
	iq_task/iq_task.o \
	iq_task/iq_task_id.o \
	iq_ui/iq_ui.o


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
MDIR ?= $(KERNELDIR)/_install_modules/lib/modules/$(KERVER)/hdal
MODPATH := $(shell echo $(PWD) | awk -F'source/' '{print $$NF}';)
MODNAME := $(shell echo $(obj-m:.o=.ko))
# variables for clean object
RM_TRACER_OBJ = $(foreach n, $(OBJ:.o=), $(if $(wildcard $(n).c),$(if $(wildcard $(n).o),$(n).o,),))
RM_NO_TRACER_OBJ = $(foreach n, $(OBJ:.o=), $(if $(wildcard $(n).c),$(if $(wildcard $(n)_no_tracer.o),$(n)_no_tracer.o,),))

modules:
	@$(MAKE) -C $(KDIR) M=$(PWD) modules

modules_install:
ifeq ($(wildcard $(OBJ:.o=.c)),$(strip $(OBJ:.o=.c)))
		@mkdir -p $(PREBUILT_DIR);
		@cp --parent -t $(PREBUILT_DIR) $($(MODULE_NAME)-objs) Makefile \
		iq_dbg.c iq_dbg.h iq_main.c  iq_main.h iq_proc.c iq_proc.h iq_version.c iq_version.h iq_param/iq_param_default.c \
		../../include/iq_alg.h ../../include/iq_ui.h ../../include/iqt_api.h;
endif
	@if [ -z $(NVT_MOD_INSTALL) ]; then \
		rm -f $(MDIR)/$(MODPATH)/$(MODNAME); \
		install -m644 -b -D $(MODNAME) ${MDIR}/$(MODPATH)/$(MODNAME); \
		cd $(KDIR)/_install_modules/lib/modules/$(KERVER)/; depmod -b $(KDIR)/_install_modules/ -a $(KERVER); \
	else \
		mkdir -p $(NVT_MOD_INSTALL)/lib/modules/$(KERVER); \
		install -m644 -b -D $(MODNAME) $(NVT_MOD_INSTALL)/lib/modules/$(KERVER)/hdal/$(MODPATH)/$(MODNAME); \
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

###############################################################################
# rtos Makefile                                                               #
###############################################################################
else ifeq ($(NVT_PRJCFG_CFG),rtos)
OUTPUT_NAME = lib$(MODULE_NAME).a
EXTRA_INCLUDE += -I$(KDRV_DIR)/include/plat
C_CFLAGS = $(PLATFORM_CFLAGS) $(EXTRA_INCLUDE)

SRC = \
	iq_dbg.c \
	iq_sxcmd.c \
	iq_version.c \
	iq_lib/iq_alg.c \
	iq_lib/iq_lib.c \
	iq_lib/iq_msg.c \
	iq_lib/iq_common_param.c \
	iq_lib/iq_flow.c \
	iq_lib/iqt_api.c \
	iq_lib/iqt_cfg.c \
	iq_lib/iqt_dtsi.c \
	iq_param/iq_param_default.c \
	iq_task/iq_task.c \
	iq_task/iq_task_id.c \
	iq_ui/iq_ui.c

copy = if [ ! -z "$(1)" -a "$(1)" != " " ]; then cp -avf $(1) $(2); fi

OBJ := $(SRC:.c=.o)

ifneq ($(wildcard $(SRC)),$(strip $(SRC)))
modules:
	@echo "nothing to be done for '$(OUTPUT_NAME)'"
clean:
	@echo "nothing to be done for '$(OUTPUT_NAME)'"
else
modules : $(OUTPUT_NAME)

%.o:%.c
	@echo Compiling $<
	@$(CC) $(C_CFLAGS) -c $< -o $@

$(OUTPUT_NAME): $(OBJ)
	@echo Creating library $* ...
	@$(AR) rcsD $(OUTPUT_NAME) $(OBJ)
	@$(BUILD_DIR)/nvt-tools/nvt-ld-op --arc-sha1 $@

clean:
	@rm -f $(OBJ) *.a *.so*
endif

modules_install:
ifneq ($(SRC),)
	@mkdir -p $(RTOS_OUTPUT_DIR)
	@cp -avf *.a $(RTOS_OUTPUT_DIR)
ifeq ($(wildcard $(SRC)),$(strip $(SRC)))
	@cp -avf -t $(PREBUILT_DIR) *.a Makefile
endif
else
	@echo "nothing to release for '$(OUTPUT_NAME)'"
endif

endif
