MODULE_NAME = kflow_videocapture
VOS_DRIVER_DIR = $(NVT_VOS_DIR)/drivers
KDRV_DIR=$(NVT_HDAL_DIR)/drivers/k_driver
KFLOW_DIR=$(NVT_HDAL_DIR)/drivers/k_flow
CURR_DRIVER_DIR = $(KFLOW_DIR)
RTOS_OUTPUT_DIR = $(CURR_DRIVER_DIR)/output
EXTRA_INCLUDE += \
	-I$(VOS_DRIVER_DIR)/include \
	-I$(KDRV_DIR)/include \
	-I$(KFLOW_DIR)/include \
	-I$(CURR_DRIVER_DIR)/source/kflow_videocapture/ctl_sen/lib \
	-I$(CURR_DRIVER_DIR)/source/kflow_videocapture/ctl_sen/include/ \
	-I$(CURR_DRIVER_DIR)/source/kflow_videocapture/ctl_sen/lib/ \
	-I$(CURR_DRIVER_DIR)/source/kflow_videocapture/ctl_sen/lib/sen_ctrl/ \
	-I$(CURR_DRIVER_DIR)/source/kflow_videocapture/ctl_sen/lib/sen_ctrl_cmdif/ \
	-I$(CURR_DRIVER_DIR)/source/kflow_videocapture/ctl_sen/lib/sen_ctrl_drv/ \
	-I$(CURR_DRIVER_DIR)/source/kflow_videocapture/ctl_sen/lib/sen_ctrl_if/ \
	-I$(CURR_DRIVER_DIR)/source/kflow_videocapture/ctl_sen/lib/sen_dbg/ \
	-I$(CURR_DRIVER_DIR)/source/kflow_videocapture/ctl_sen/lib/sen_middle/ \
	-I$(CURR_DRIVER_DIR)/source/kflow_videocapture/ctl_sen/lib/sen_utility/ \
	-I$(CURR_DRIVER_DIR)/source/kflow_videocapture/ctl_sie/include/ \
	-I$(CURR_DRIVER_DIR)/source/kflow_videocapture/ctl_sie/sie_debug/ \
	-I$(CURR_DRIVER_DIR)/source/kflow_videocapture/ctl_sie/sie_utility/ \
	-I$(CURR_DRIVER_DIR)/source/kflow_videocapture/ctl_sie/sie_flow/ \

.PHONY: modules modules_install clean
###############################################################################
# Linux Makefile                                                              #
###############################################################################
ifeq ($(NVT_PRJCFG_CFG),Linux)
C_CFLAGS  += -D_ARCH_ARM_=1 -D_ARCH_MIPS_=0 -D_ARCH_=$(_ARCH_ARM_)
C_CFLAGS  += -D__LINUX -Werror
EXTRA_CFLAGS += $(C_CFLAGS) $(EXTRA_INCLUDE) -Wno-date-time -D__SOC_680_PLATFORM__ -DDEBUG -D_GROUP_KO_ -Wno-missing-braces
KBUILD_EXTRA_SYMBOLS = $(shell find $(NVT_HDAL_DIR)/drivers -name Module.symvers)
ccflags-y  := $(EXTRA_CFLAGS)

ifeq ($(CONFIG_NVT_HDAL_KFLOW_VIDEOCAPTURE),y)
obj-$(CONFIG_NVT_HDAL_KFLOW_VIDEOCAPTURE) = $(MODULE_NAME).o
else
obj-m += $(MODULE_NAME).o
endif

OBJ := \
	kflow_videocapture_main.o \
	ctl_sen/ctl_sen_main.o \
	ctl_sen/ctl_sen_proc.o \
	ctl_sen/ctl_sen_drv.o \
	ctl_sen/ctl_sen_api.o \
	ctl_sen/ctl_sen_config.o \
	ctl_sen/lib/ctl_sen.o \
	ctl_sen/lib/sen_ctrl/sen_ctrl.o \
	ctl_sen/lib/sen_ctrl/sen_ctrl_clk.o \
	ctl_sen/lib/sen_ctrl_cmdif/sen_ctrl_cmdif.o \
	ctl_sen/lib/sen_ctrl_cmdif/sen_ctrl_cmdif_vx1.o \
	ctl_sen/lib/sen_ctrl_if/sen_ctrl_if.o \
	ctl_sen/lib/sen_ctrl_if/sen_ctrl_if_csi.o \
	ctl_sen/lib/sen_ctrl_if/sen_ctrl_if_lvds.o \
	ctl_sen/lib/sen_ctrl_if/sen_ctrl_if_slvsec.o \
	ctl_sen/lib/sen_ctrl_if/sen_ctrl_if_tge.o \
	ctl_sen/lib/sen_ctrl_if/sen_ctrl_if_glb.o \
	ctl_sen/lib/sen_ctrl_drv/sen_ctrl_drv.o \
	ctl_sen/lib/sen_middle/sen_middle.o \
	ctl_sen/lib/sen_utility/sen_utility.o \
	ctl_sen/lib/sen_utility/sen_utility_clk.o \
	ctl_sen/lib/sen_utility/sen_id_map.o \
	ctl_sen/lib/sen_dbg/sen_dbg_infor.o \
	ctl_sie/ctl_sie_main.o \
	ctl_sie/ctl_sie_proc.o \
	ctl_sie/ctl_sie_drv.o \
	ctl_sie/ctl_sie_api.o \
	ctl_sie/ctl_sie_export.o \
	ctl_sie/sie_flow/kdf_sie.o \
	ctl_sie/sie_utility/ctl_sie_event_id.o \
	ctl_sie/sie_utility/ctl_sie_event.o \
	ctl_sie/sie_utility/ctl_sie_utility_int.o \
	ctl_sie/sie_debug/ctl_sie_debug_int.o \
	ctl_sie/sie_flow/ctl_sie_buf.o \
	ctl_sie/sie_flow/ctl_sie_iosize.o \
	ctl_sie/sie_flow/ctl_sie_id.o \
	ctl_sie/sie_flow/ctl_sie_isp_task.o \
	ctl_sie/sie_flow/ctl_sie.o \
	ctl_sie/sie_flow/ctl_sie_isp.o \
	ctl_sie/ctl_sie_ver.o \
	unit/linux/isf_vdocap_main.o \
	unit/linux/isf_vdocap_proc.o \
	unit/linux/isf_vdocap_drv.o \
	unit/isf_vdocap_api.o \
	unit/isf_vdocap.o \
	unit/isf_vdocap_debug.o \
	unit/isf_vdocap_id.o \
	unit/isf_vdocap_output.o \
	unit/isf_vdocap_pull.o \
	unit/isf_vdocap0.o \
	unit/isf_vdocap1.o \
	unit/isf_vdocap2.o \
	unit/isf_vdocap3.o \
	unit/isf_vdocap4.o \
	unit/isf_vdocap_verinfo.o \

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
C_CFLAGS = $(PLATFORM_CFLAGS) $(EXTRA_INCLUDE) -DDEBUG

SRC = \
	kflow_videocapture_main.c \
	ctl_sen/ctl_sen_config.c \
	ctl_sen/ctl_sen_api.c \
	ctl_sen/lib/ctl_sen.c \
	ctl_sen/lib/sen_ctrl/sen_ctrl.c \
	ctl_sen/lib/sen_ctrl/sen_ctrl_clk.c \
	ctl_sen/lib/sen_ctrl_cmdif/sen_ctrl_cmdif.c \
	ctl_sen/lib/sen_ctrl_cmdif/sen_ctrl_cmdif_vx1.c \
	ctl_sen/lib/sen_ctrl_drv/sen_ctrl_drv.c \
	ctl_sen/lib/sen_ctrl_if/sen_ctrl_if.c \
	ctl_sen/lib/sen_ctrl_if/sen_ctrl_if_csi.c \
	ctl_sen/lib/sen_ctrl_if/sen_ctrl_if_lvds.c \
	ctl_sen/lib/sen_ctrl_if/sen_ctrl_if_slvsec.c \
	ctl_sen/lib/sen_ctrl_if/sen_ctrl_if_tge.c \
	ctl_sen/lib/sen_ctrl_if/sen_ctrl_if_glb.c \
	ctl_sen/lib/sen_dbg/sen_dbg_infor.c \
	ctl_sen/lib/sen_middle/sen_middle.c \
	ctl_sen/lib/sen_utility/sen_id_map.c \
	ctl_sen/lib/sen_utility/sen_utility.c \
	ctl_sen/lib/sen_utility/sen_utility_clk.c \
	ctl_sie/ctl_sie_api.c \
	ctl_sie/sie_flow/kdf_sie.c \
	ctl_sie/sie_utility/ctl_sie_event_id.c \
	ctl_sie/sie_utility/ctl_sie_event.c \
	ctl_sie/sie_utility/ctl_sie_utility_int.c \
	ctl_sie/sie_debug/ctl_sie_debug_int.c \
	ctl_sie/sie_flow/ctl_sie_buf.c \
	ctl_sie/sie_flow/ctl_sie_iosize.c \
	ctl_sie/sie_flow/ctl_sie_id.c \
	ctl_sie/sie_flow/ctl_sie_isp_task.c \
	ctl_sie/sie_flow/ctl_sie.c \
	ctl_sie/sie_flow/ctl_sie_isp.c \
	ctl_sie/ctl_sie_ver.c \
	unit/rtos/isf_vdocap_drv.c \
	unit/rtos/isf_vdocap_proc.c \
	unit/isf_vdocap_api.c \
	unit/isf_vdocap.c \
	unit/isf_vdocap_debug.c \
	unit/isf_vdocap_id.c \
	unit/isf_vdocap_output.c \
	unit/isf_vdocap_pull.c \
	unit/isf_vdocap0.c \
	unit/isf_vdocap1.c \
	unit/isf_vdocap2.c \
	unit/isf_vdocap3.o \
	unit/isf_vdocap4.o \
	unit/isf_vdocap_verinfo.c

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