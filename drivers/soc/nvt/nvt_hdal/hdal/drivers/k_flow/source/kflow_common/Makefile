MODULE_NAME = kflow_common
VOS_DRIVER_DIR = $(NVT_VOS_DIR)/drivers
KDRV_DIR=$(NVT_HDAL_DIR)/drivers/k_driver
KFLOW_DIR=$(NVT_HDAL_DIR)/drivers/k_flow
CURR_DRIVER_DIR = $(KFLOW_DIR)
RTOS_OUTPUT_DIR = $(CURR_DRIVER_DIR)/output
EXTRA_INCLUDE += \
	-I$(VOS_DRIVER_DIR)/include \
	-I$(KDRV_DIR)/include/plat \
	-I$(KDRV_DIR)/include/comm \
	-I$(KDRV_DIR)/include \
	-I$(KFLOW_DIR)/include \
	-I$(KDRV_DIR)/source/kdrv_gfx2d/kdrv_ise/include/ \
	-I$(KDRV_DIR)/source/kdrv_videoout/include/ \
	-I$(KDRV_DIR)/include/kdrv_builtin \

.PHONY: modules modules_install clean
###############################################################################
# Linux Makefile                                                              #
###############################################################################
ifeq ($(NVT_PRJCFG_CFG),Linux)
C_CFLAGS  += -D_ARCH_ARM_=1 -D_ARCH_MIPS_=0 -D_ARCH_=$(_ARCH_ARM_)
C_CFLAGS  += -D__LINUX -Werror
EXTRA_CFLAGS += $(C_CFLAGS) $(EXTRA_INCLUDE) -Wno-date-time -D__SOC_680_PLATFORM__ -DDEBUG -D_GROUP_KO_
KBUILD_EXTRA_SYMBOLS = $(shell find $(NVT_HDAL_DIR)/drivers -name Module.symvers)
ccflags-y  := $(EXTRA_CFLAGS)

ifeq ($(CONFIG_NVT_HDAL_KFLOW_COMMON),y)
obj-$(CONFIG_NVT_HDAL_KFLOW_COMMON) = $(MODULE_NAME).o
else
obj-m += $(MODULE_NAME).o
endif

OBJ := \
	kflow_common_main.o \
	nvtmpp/nvtmpp_api.o \
	nvtmpp/nvtmpp_pool.o \
	nvtmpp/nvtmpp_heap2.o \
	nvtmpp/nvtmpp_blk.o \
	nvtmpp/nvtmpp_id.o \
	nvtmpp/nvtmpp_log.o \
	nvtmpp/nvtmpp_module.o \
	nvtmpp/nvtmpp_main.o \
	nvtmpp/nvtmpp_proc.o \
	nvtmpp/nvtmpp_ioctl.o \
	nvtmpp/nvtmpp_debug_cmd.o \
	nvtmpp/nvtmpp_export.o \
	nvtmpp/nvtmpp_platform.o \
	isf_flow/linux/isf_flow_main.o \
	isf_flow/linux/isf_flow_proc.o \
	isf_flow/linux/isf_flow_drv.o \
	isf_flow/linux/isf_flow_debug_log.o \
	isf_flow/isf_flow_api.o \
	isf_flow/isf_flow_core.o \
	isf_flow/isf_flow_queue.o \
	isf_flow/isf_flow_debug.o \
	isf_flow/isf_flow_id.o \
	isf_flow/isf_flow_unit_data.o \
	isf_flow/isf_flow_unit_param.o \
	isf_flow/isf_flow_unit_state.o \
	isf_flow/isf_flow_data_common.o \
	isf_flow/isf_flow_data_vdofm.o \
	isf_flow/isf_flow_data_vdobs.o \
	isf_flow/isf_flow_data_audfm.o \
	isf_flow/isf_flow_data_audbs.o \
	isf_flow/isf_flow_verinfo.o \
	gximage/gximage.o \
	gximage/gximage_open.o \
	fastboot/fastboot_main.o \
	fastboot/fastboot_proc.o \
	fastboot/fastboot_fdt.o \


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
MODPATH := $(shell echo $(PWD) | awk -F'linux-driver/' '{print $$NF}')
MODNAME := $(shell echo $(obj-m:.o=.ko))
# variables for clean object
RM_TRACER_OBJ = $(foreach n, $(OBJ:.o=), $(if $(wildcard $(n).c),$(if $(wildcard $(n).o),$(n).o,),))
RM_NO_TRACER_OBJ = $(foreach n, $(OBJ:.o=), $(if $(wildcard $(n).c),$(if $(wildcard $(n)_no_tracer.o),$(n)_no_tracer.o,),))

modules:
	$(MAKE) -C $(KDIR) M=$(PWD) $(NVT_KGCOV) modules

modules_install:
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
# -Wno-format for Jeah's isf_flow compiler error
C_CFLAGS = $(PLATFORM_CFLAGS) $(EXTRA_INCLUDE) -DDEBUG -Wno-format

SRC = \
	kflow_common_main.c \
	nvtmpp/nvtmpp_api.c \
	nvtmpp/nvtmpp_pool.c \
	nvtmpp/nvtmpp_heap2.c \
	nvtmpp/nvtmpp_blk.c \
	nvtmpp/nvtmpp_id.c \
	nvtmpp/nvtmpp_log.c \
	nvtmpp/nvtmpp_module.c \
	nvtmpp/nvtmpp_ioctl.c \
	nvtmpp/nvtmpp_debug_cmd.c \
	nvtmpp/nvtmpp_platform.c \
	isf_flow/rtos/isf_flow_drv.c \
	isf_flow/rtos/isf_flow_debug_log.c \
	isf_flow/isf_flow_api.c \
	isf_flow/isf_flow_core.c \
	isf_flow/isf_flow_queue.c \
	isf_flow/isf_flow_debug.c \
	isf_flow/isf_flow_id.c \
	isf_flow/isf_flow_unit_data.c \
	isf_flow/isf_flow_unit_param.c \
	isf_flow/isf_flow_unit_state.c \
	isf_flow/isf_flow_data_common.c \
	isf_flow/isf_flow_data_vdofm.c \
	isf_flow/isf_flow_data_vdobs.c \
	isf_flow/isf_flow_data_audfm.c \
	isf_flow/isf_flow_data_audbs.c \
	gximage/gximage.o \
	gximage/gximage_open.o

copy = if [ ! -z "$(1)" -a "$(1)" != " " ]; then cp -avf $(1) $(2); fi

OBJ = $(SRC:.c=.o)

ifeq ("$(wildcard *.c */*.c */*/*.c)","")
modules:
	@echo "nothing to be done for '$(MODULE_NAME)'"

clean:
	@echo "nothing to be done for '$(MODULE_NAME)'"
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
	@mkdir -p $(RTOS_OUTPUT_DIR)
	@cp -avf *.a $(RTOS_OUTPUT_DIR)

endif
