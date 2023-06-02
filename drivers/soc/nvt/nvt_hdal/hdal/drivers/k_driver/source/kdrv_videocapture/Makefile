MODULE_NAME = kdrv_videocapture
VOS_DRIVER_DIR = $(NVT_VOS_DIR)/drivers
KDRV_DIR = $(NVT_HDAL_DIR)/drivers/k_driver
CURR_DRIVER_DIR = $(KDRV_DIR)
RTOS_OUTPUT_DIR = $(CURR_DRIVER_DIR)/output
EXTRA_INCLUDE += \
	-I$(VOS_DRIVER_DIR)/include \
	-I$(KDRV_DIR)/include \
	-I$(KDRV_DIR)/source/kdrv_builtin/include \
	-I$(KDRV_DIR)/source/kdrv_builtin/sie \
	-I$(KDRV_DIR)/include/kdrv_videocapture \
	-I$(LIBRARY_DIR)/include \
	-I$(CURR_DRIVER_DIR)/source/include \
	-I$(CURR_DRIVER_DIR)/source/kdrv_videocapture/kdrv_sie/include \
	-I$(CURR_DRIVER_DIR)/source/kdrv_videocapture/kdrv_sie/kdrv_flow \
	-I$(CURR_DRIVER_DIR)/source/kdrv_videocapture/kdrv_sie/kdrv_debug \
	-I$(CURR_DRIVER_DIR)/source/kdrv_videocapture/kdrv_tge/include \
	-I$(CURR_DRIVER_DIR)/source/kdrv_videocapture/kdrv_tge/eng/include \
	-I$(CURR_DRIVER_DIR)/source/kdrv_videocapture/ssenif/hal/include \
	-I$(CURR_DRIVER_DIR)/source/kdrv_videocapture/ssenif/hal \
	-I$(CURR_DRIVER_DIR)/source/kdrv_videocapture/ssenif/senphy/include \
	-I$(CURR_DRIVER_DIR)/source/kdrv_videocapture/ssenif/csi/include \
	-I$(CURR_DRIVER_DIR)/source/kdrv_videocapture/ssenif/lvds/include \

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

ifeq ($(CONFIG_NVT_HDAL_KDRV_VIDEOCAPTURE),y)
obj-$(CONFIG_NVT_HDAL_KDRV_VIDEOCAPTURE) = $(MODULE_NAME).o
else
obj-m += $(MODULE_NAME).o
endif

OBJ := \
	kdrv_videocapture_main.o \
	kdrv_sie/kdrv_sie_main.o \
	kdrv_sie/kdrv_sie_proc.o \
	kdrv_sie/kdrv_sie_api.o \
	kdrv_sie/kdrv_sie_export.o \
	kdrv_sie/kdrv_flow/kdrv_sie.o \
	kdrv_sie/kdrv_flow/kdrv_sie_config.o \
	kdrv_sie/kdrv_debug/kdrv_sie_debug_int.o \
	kdrv_tge/eng/tge_int.o \
	kdrv_tge/eng/tge_lib.o \
	kdrv_tge/eng/tge_platform.o \
	kdrv_tge/eng/tge_drv.o \
	kdrv_tge/kdrv_tge_main.o \
	kdrv_tge/kdrv_tge_proc.o \
	kdrv_tge/kdrv_tge_drv.o \
	kdrv_tge/kdrv_tge_api.o \
	kdrv_tge/kdrv_tge.o \
	kdrv_tge/kdrv_tge_export.o \
	ssenif/hal/ssenif_main.o \
	ssenif/hal/ssenif_proc.o \
	ssenif/hal/ssenif_drv.o \
	ssenif/hal/ssenif_api.o \
	ssenif/hal/kdrv_ssenif.o \
	ssenif/hal/dal_ssenif.o \
	ssenif/hal/serial_sen_intf_lvds.o \
	ssenif/hal/serial_sen_intf_lvds2.o \
	ssenif/hal/serial_sen_intf_csi.o \
	ssenif/hal/serial_sen_intf_csi2.o \
	ssenif/senphy/senphy_main.o \
	ssenif/senphy/senphy_proc.o \
	ssenif/senphy/senphy_drv.o \
	ssenif/senphy/senphy_api.o \
	ssenif/senphy/senphy.o \
	ssenif/csi/csi_main.o \
	ssenif/csi/csi_proc.o \
	ssenif/csi/csi_drv.o \
	ssenif/csi/csi_api.o \
	ssenif/csi/csi_obj.o \
	ssenif/csi/csi.o \
	ssenif/csi/csi2.o \
	ssenif/csi/csi_platform.o\
	ssenif/lvds/lvds_main.o \
	ssenif/lvds/lvds_proc.o \
	ssenif/lvds/lvds_drv.o \
	ssenif/lvds/lvds_api.o \
	ssenif/lvds/lvds.o \
	ssenif/lvds/lvds2.o \
	ssenif/lvds/lvdsobj.o \

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
	kdrv_videocapture_main.c \
	kdrv_sie/kdrv_flow/kdrv_sie.c \
	kdrv_sie/kdrv_flow/kdrv_sie_config.c \
	kdrv_sie/kdrv_debug/kdrv_sie_debug_int.c \
	kdrv_sie/kdrv_sie_api.c \
	kdrv_tge/eng/tge_int.c \
	kdrv_tge/eng/tge_lib.c \
	kdrv_tge/eng/tge_platform.c \
	kdrv_tge/kdrv_tge_api.c \
	kdrv_tge/kdrv_tge.c \
	ssenif/hal/kdrv_ssenif.c \
	ssenif/hal/dal_ssenif.c \
	ssenif/hal/serial_sen_intf_lvds.c \
	ssenif/hal/serial_sen_intf_lvds2.c \
	ssenif/hal/serial_sen_intf_csi.c \
	ssenif/hal/serial_sen_intf_csi2.c \
	ssenif/senphy/senphy.c \
	ssenif/csi/csi.c \
	ssenif/csi/csi2.c \
	ssenif/csi/csi_obj.c \
	ssenif/csi/csi_platform.c\
	ssenif/lvds/lvds.c \
	ssenif/lvds/lvds2.c \
	ssenif/lvds/lvdsobj.c \

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
