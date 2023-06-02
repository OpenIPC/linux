MODULE_NAME = kflow_videoprocess
VOS_DRIVER_DIR = $(NVT_VOS_DIR)/drivers
KDRV_DIR=$(NVT_HDAL_DIR)/drivers/k_driver
KFLOW_DIR=$(NVT_HDAL_DIR)/drivers/k_flow
CURR_DRIVER_DIR = $(KFLOW_DIR)
RTOS_OUTPUT_DIR = $(CURR_DRIVER_DIR)/output
EXTRA_INCLUDE += \
	-I$(VOS_DRIVER_DIR)/include \
	-I$(KDRV_DIR)/include \
	-I$(KDRV_DIR)/include/comm \
	-I$(KFLOW_DIR)/include \
	-I$(CURR_DRIVER_DIR)/source/kflow_videoprocess/ctl_ipp/include \
	-I$(CURR_DRIVER_DIR)/source/kflow_videoprocess/ctl_ipp/ipp_debug \
	-I$(CURR_DRIVER_DIR)/source/kflow_videoprocess/ctl_ipp/ipp_utility \
	-I$(CURR_DRIVER_DIR)/source/kflow_videoprocess/ctl_ipp/ipp_flow \
	-I$(CURR_DRIVER_DIR)/source/kflow_videoprocess/ctl_ise/include \

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

ifeq ($(CONFIG_NVT_HDAL_KFLOW_VIDEOPROCESS),y)
obj-$(CONFIG_NVT_HDAL_KFLOW_VIDEOPROCESS) = $(MODULE_NAME).o
else
obj-m += $(MODULE_NAME).o
endif

OBJ := \
	kflow_videoprocess_main.o \
	ctl_ipp/ctl_ipp_main.o \
	ctl_ipp/ctl_ipp_proc.o \
	ctl_ipp/ctl_ipp_drv.o \
	ctl_ipp/ctl_ipp_api.o \
	ctl_ipp/ctl_ipp_verinfo.o \
	ctl_ipp/ipp_utility/ctl_ipp_util.o \
	ctl_ipp/ipp_utility/ipp_event_id.o \
	ctl_ipp/ipp_utility/ipp_event.o \
	ctl_ipp/ipp_flow/ctl_ipp_isp_int.o \
	ctl_ipp/ipp_flow/ctl_ipp_isp.o \
	ctl_ipp/ipp_flow/ctl_ipp_buf.o \
	ctl_ipp/ipp_flow/ctl_ipp_id.o \
	ctl_ipp/ipp_flow/ctl_ipp_int.o \
	ctl_ipp/ipp_flow/ctl_ipp.o \
	ctl_ipp/ipp_flow/ctl_ipp_flow_task.o \
	ctl_ipp/ipp_debug/ipp_debug_int.o \
	ctl_ipp/ctl_ipp_export.o \
	ctl_ipp/ctl_ipp_platform.o \
	ctl_ise/ctl_ise_main.o \
	ctl_ise/ctl_ise_proc.o \
	ctl_ise/ctl_ise_drv.o \
	ctl_ise/ctl_ise_api.o \
	ctl_ise/ise_flow/ctl_ise_int.o \
	ctl_ise/ise_flow/ctl_ise.o \
	ctl_ise/ctl_ise_export.o \
	unit/linux/isf_vdoprc_main.o \
	unit/linux/isf_vdoprc_proc.o \
	unit/linux/isf_vdoprc_drv.o \
	unit/isf_vdoprc_api.o \
	unit/isf_vdoprc_id.o \
	unit/isf_vdoprc.o \
	unit/isf_vdoprc_input.o \
	unit/isf_vdoprc_input_mask.o \
	unit/isf_vdoprc_input_osd.o \
	unit/isf_vdoprc_input_vpe.o \
	unit/isf_vdoprc_input_ise.o \
	unit/isf_vdoprc_process.o \
	unit/isf_vdoprc_output.o \
	unit/isf_vdoprc_output_ext.o \
	unit/isf_vdoprc_output_ext_tsk.o \
	unit/isf_vdoprc_output_mask.o \
	unit/isf_vdoprc_output_osd.o \
	unit/isf_vdoprc_output_vpe.o \
	unit/isf_vdoprc_output_ise.o \
	unit/isf_vdoprc_pull.o \
	unit/isf_vdoprc_debug.o \
	unit/isf_vdoprc0.o \
	unit/isf_vdoprc1.o \
	unit/isf_vdoprc2.o \
	unit/isf_vdoprc3.o \
	unit/isf_vdoprc4.o \
	unit/isf_vdoprc5.o \
	unit/isf_vdoprc6.o \
	unit/isf_vdoprc7.o \
	unit/isf_vdoprc8.o \
	unit/isf_vdoprc9.o \
	unit/isf_vdoprc10.o \
	unit/isf_vdoprc11.o \
	unit/isf_vdoprc12.o \
	unit/isf_vdoprc13.o \
	unit/isf_vdoprc14.o \
	unit/isf_vdoprc15.o \
	unit/isf_vdoprc_verinfo.o \

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
	kflow_videoprocess_main.c \
	ctl_ipp/ipp_debug/ipp_debug_int.c \
	ctl_ipp/ipp_utility/ctl_ipp_util.c \
	ctl_ipp/ipp_utility/ipp_event_id.c \
	ctl_ipp/ipp_utility/ipp_event.c \
	ctl_ipp/ipp_flow/ctl_ipp_isp_int.c \
	ctl_ipp/ipp_flow/ctl_ipp_isp.c \
	ctl_ipp/ipp_flow/ctl_ipp_buf.c \
	ctl_ipp/ipp_flow/ctl_ipp_id.c \
	ctl_ipp/ipp_flow/ctl_ipp_int.c \
	ctl_ipp/ipp_flow/ctl_ipp.c \
	ctl_ipp/ipp_flow/ctl_ipp_flow_task.c \
	ctl_ipp/ctl_ipp_api.c \
	ctl_ipp/ctl_ipp_platform.c \
	ctl_ise/ctl_ise_api.c \
	ctl_ise/ise_flow/ctl_ise_int.c \
	ctl_ise/ise_flow/ctl_ise.c \
	unit/rtos/isf_vdoprc_drv.o \
	unit/rtos/isf_vdoprc_proc.o \
	unit/isf_vdoprc_api.o \
	unit/isf_vdoprc_id.o \
	unit/isf_vdoprc.o \
	unit/isf_vdoprc_input.o \
	unit/isf_vdoprc_input_mask.o \
	unit/isf_vdoprc_input_osd.o \
	unit/isf_vdoprc_input_ise.o \
	unit/isf_vdoprc_process.o \
	unit/isf_vdoprc_output.o \
	unit/isf_vdoprc_output_ext.o \
	unit/isf_vdoprc_output_ext_tsk.o \
	unit/isf_vdoprc_output_mask.o \
	unit/isf_vdoprc_output_osd.o \
	unit/isf_vdoprc_output_ise.o \
	unit/isf_vdoprc_pull.o \
	unit/isf_vdoprc_debug.o \
	unit/isf_vdoprc0.o \
	unit/isf_vdoprc1.o \
	unit/isf_vdoprc2.o \
	unit/isf_vdoprc3.o \
	unit/isf_vdoprc4.o \
	unit/isf_vdoprc5.o \
	unit/isf_vdoprc6.o \
	unit/isf_vdoprc7.o \
	unit/isf_vdoprc8.o \
	unit/isf_vdoprc9.o \
	unit/isf_vdoprc10.o \
	unit/isf_vdoprc11.o \
	unit/isf_vdoprc12.o \
	unit/isf_vdoprc13.o \
	unit/isf_vdoprc14.o \
	unit/isf_vdoprc15.o \
	unit/isf_vdoprc_verinfo.o

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
