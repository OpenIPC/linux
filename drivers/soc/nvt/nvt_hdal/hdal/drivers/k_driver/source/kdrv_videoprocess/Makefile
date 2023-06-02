MODULE_NAME = kdrv_videoprocess
VOS_DRIVER_DIR = $(NVT_VOS_DIR)/drivers
KDRV_DIR = $(NVT_HDAL_DIR)/drivers/k_driver
CURR_DRIVER_DIR = $(KDRV_DIR)
RTOS_OUTPUT_DIR = $(CURR_DRIVER_DIR)/output
EXTRA_INCLUDE += \
	-I$(VOS_DRIVER_DIR)/include \
	-I$(KDRV_DIR)/include \
	-I$(KDRV_DIR)/include/comm \
	-I$(CURR_DRIVER_DIR)/source/include \
	-I$(CURR_DRIVER_DIR)/source/kdrv_videoprocess/kdrv_dce/include \
	-I$(CURR_DRIVER_DIR)/source/kdrv_videoprocess/kdrv_dce/eng/include \
	-I$(CURR_DRIVER_DIR)/source/kdrv_videoprocess/kdrv_ife/include \
	-I$(CURR_DRIVER_DIR)/source/kdrv_videoprocess/kdrv_ife/eng/include \
	-I$(CURR_DRIVER_DIR)/source/kdrv_videoprocess/kdrv_ime/include \
	-I$(CURR_DRIVER_DIR)/source/kdrv_videoprocess/kdrv_ime/eng/include \
	-I$(CURR_DRIVER_DIR)/source/kdrv_videoprocess/kdrv_ipe/include \
	-I$(CURR_DRIVER_DIR)/source/kdrv_videoprocess/kdrv_ipe/eng/include \
	-I$(CURR_DRIVER_DIR)/source/kdrv_videoprocess/kdrv_ipp/include \
	-I$(CURR_DRIVER_DIR)/source/kdrv_videoprocess/kdrv_sde/include \
	-I$(CURR_DRIVER_DIR)/source/kdrv_builtin/ipp/include \
	-I$(KDRV_DIR)/include/kdrv_builtin
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

ifeq ($(CONFIG_NVT_HDAL_KDRV_VIDEOPROCESS),y)
obj-$(CONFIG_NVT_HDAL_KDRV_VIDEOPROCESS) = $(MODULE_NAME).o
else
obj-m += $(MODULE_NAME).o
endif

OBJ := \
	kdrv_videoprocess_main.o \
	kdrv_ife/eng/ife_eng_platform.o \
	kdrv_ife/eng/ife_eng.o \
	kdrv_ife/eng/ife_eng_base.o \
	kdrv_ife/eng/ife_eng_int.o \
	kdrv_ife/kdrv_ife_main.o \
	kdrv_ife/kdrv_ife_proc.o \
	kdrv_ife/kdrv_ife_init.o \
	kdrv_ife/kdrv_ife_api.o \
	kdrv_ife/kdrv_ife_platform.o \
	kdrv_ife/kdrv_ife.o \
	kdrv_ime/eng/ime_eng_int_platform.o \
	kdrv_ime/eng/ime_eng.o \
	kdrv_ime/eng/ime_eng_3dnr_base.o \
	kdrv_ime/eng/ime_eng_compression_base.o \
	kdrv_ime/eng/ime_eng_control_base.o \
	kdrv_ime/eng/ime_eng_dbcs_base.o \
	kdrv_ime/eng/ime_eng_in_out_path_base.o \
	kdrv_ime/eng/ime_eng_cal.o \
	kdrv_ime/eng/ime_eng_lca_base.o \
	kdrv_ime/eng/ime_eng_lcaf_base.o \
	kdrv_ime/eng/ime_eng_osd_base.o \
	kdrv_ime/eng/ime_eng_pm_base.o \
	kdrv_ime/eng/ime_eng_yuvcvt_base.o \
	kdrv_ime/eng/ime_eng_limt.o \
	kdrv_ime/kdrv_ime_init.o \
	kdrv_ime/kdrv_ime_main.o \
	kdrv_ime/kdrv_ime_proc.o \
	kdrv_ime/kdrv_ime_api.o \
	kdrv_ime/kdrv_ime_platform.o \
	kdrv_ime/kdrv_ime.o \
	kdrv_ipe/eng/ipe_eng_platform.o \
	kdrv_ipe/eng/ipe_eng_int.o \
	kdrv_ipe/eng/ipe_eng_base.o \
	kdrv_ipe/eng/ipe_eng.o \
	kdrv_ipe/eng/ipe_eng_limt.o \
	kdrv_ipe/kdrv_ipe_init.o \
	kdrv_ipe/kdrv_ipe_main.o \
	kdrv_ipe/kdrv_ipe_proc.o \
	kdrv_ipe/kdrv_ipe_api.o \
	kdrv_ipe/kdrv_ipe_platform.o \
	kdrv_ipe/kdrv_ipe.o \
	kdrv_dce/eng/dce_eng_platform.o \
	kdrv_dce/eng/dce_eng.o \
	kdrv_dce/eng/dce_eng_int.o \
	kdrv_dce/kdrv_dce_main.o \
	kdrv_dce/kdrv_dce_proc.o \
	kdrv_dce/kdrv_dce_api.o \
	kdrv_dce/kdrv_dce_platform.o \
	kdrv_dce/kdrv_dce.o \
	kdrv_dce/kdrv_dce_init.o \
	kdrv_ipp/kdrv_ipp_main.o \
	kdrv_ipp/kdrv_ipp_proc.o \
	kdrv_ipp/kdrv_ipp_api.o \
	kdrv_ipp/kdrv_ipp_int.o \
	kdrv_ipp/kdrv_ipp_util.o \
	kdrv_ipp/kdrv_ipp_export.o \
	kdrv_ipp/kdrv_ipp_platform.o \
	kdrv_ipp/kdrv_ipp.o \
	#kdrv_sde/sde_main.o \
	#kdrv_sde/sde_proc.o \
	#kdrv_sde/sde_drv.o \
	#kdrv_sde/sde_api.o \
	#kdrv_sde/sde.o \
	#kdrv_sde/kdrv_sde_config.o \
	#kdrv_sde/kdrv_sde_api.o \
	#kdrv_sde/sde_platform.o

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
EXTRA_INCLUDE += -I$(KDRV_DIR)/include/plat
C_CFLAGS = $(PLATFORM_CFLAGS) $(EXTRA_INCLUDE) -DDEBUG

SRC = \
	kdrv_videoprocess_main.c \
	kdrv_ife/kdrv_ife_init.c \
	kdrv_ife/kdrv_ife_api.c \
	kdrv_ife/kdrv_ife_platform.c \
	kdrv_ife/kdrv_ife.c \
	kdrv_ife/eng/ife_eng.c \
	kdrv_ife/eng/ife_eng_base.c \
	kdrv_ife/eng/ife_eng_int.c \
	kdrv_ife/eng/ife_eng_platform.c \
	kdrv_ime/eng/ime_eng_int_platform.c \
	kdrv_ime/eng/ime_eng.c \
	kdrv_ime/eng/ime_eng_3dnr_base.c \
	kdrv_ime/eng/ime_eng_compression_base.c \
	kdrv_ime/eng/ime_eng_control_base.c \
	kdrv_ime/eng/ime_eng_dbcs_base.c \
	kdrv_ime/eng/ime_eng_in_out_path_base.c \
	kdrv_ime/eng/ime_eng_cal.c \
	kdrv_ime/eng/ime_eng_lca_base.c \
	kdrv_ime/eng/ime_eng_lcaf_base.c \
	kdrv_ime/eng/ime_eng_osd_base.c \
	kdrv_ime/eng/ime_eng_pm_base.c \
	kdrv_ime/eng/ime_eng_yuvcvt_base.c \
	kdrv_ime/eng/ime_eng_limt.c \
	kdrv_ime/kdrv_ime_init.c \
	kdrv_ime/kdrv_ime_api.c \
	kdrv_ime/kdrv_ime_platform.c \
	kdrv_ime/kdrv_ime.c \
	kdrv_ipe/eng/ipe_eng_platform.c \
	kdrv_ipe/eng/ipe_eng_int.c \
	kdrv_ipe/eng/ipe_eng_base.c \
	kdrv_ipe/eng/ipe_eng.c \
	kdrv_ipe/eng/ipe_eng_limt.c \
	kdrv_ipe/kdrv_ipe_init.c \
	kdrv_ipe/kdrv_ipe_api.c \
	kdrv_ipe/kdrv_ipe_platform.c \
	kdrv_ipe/kdrv_ipe.c \
	kdrv_dce/kdrv_dce_init.c \
	kdrv_dce/kdrv_dce_api.c \
	kdrv_dce/kdrv_dce_platform.c \
	kdrv_dce/kdrv_dce.c \
	kdrv_dce/eng/dce_eng.c \
	kdrv_dce/eng/dce_eng_platform.c \
	kdrv_dce/eng/dce_eng_int.c \
	kdrv_ipp/kdrv_ipp_api.c \
	kdrv_ipp/kdrv_ipp_int.c \
	kdrv_ipp/kdrv_ipp_util.c \
	kdrv_ipp/kdrv_ipp_platform.c \
	kdrv_ipp/kdrv_ipp.c \
	kdrv_sde/sde.c \
	kdrv_sde/sde_platform.c \
	kdrv_sde/kdrv_sde_config.c \
	kdrv_sde/kdrv_sde_api.c  \

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
