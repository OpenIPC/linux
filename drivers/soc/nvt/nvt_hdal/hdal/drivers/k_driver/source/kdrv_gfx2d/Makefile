MODULE_NAME = kdrv_gfx2d
# DIRs
VOS_DRIVER_DIR = $(NVT_VOS_DIR)/drivers
KDRV_DIR = $(NVT_HDAL_DIR)/drivers/k_driver
CURR_DRIVER_DIR = $(KDRV_DIR)
RTOS_OUTPUT_DIR = $(CURR_DRIVER_DIR)/output
# INC FLAGs
EXTRA_INCLUDE += \
	-I$(VOS_DRIVER_DIR)/include \
	-I$(KDRV_DIR)/include \
	-I$(CURR_DRIVER_DIR)/source/include \
	-I$(CURR_DRIVER_DIR)/source/kdrv_gfx2d/kdrv_grph/include \
	-I$(CURR_DRIVER_DIR)/source/kdrv_gfx2d/kdrv_hwcopy/include \
	-I$(CURR_DRIVER_DIR)/source/kdrv_gfx2d/kdrv_ise/eng/include \
	-I$(CURR_DRIVER_DIR)/source/kdrv_gfx2d/kdrv_ise/include \
	-I$(CURR_DRIVER_DIR)/source/kdrv_gfx2d/kdrv_rotation/include \

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

ifeq ($(CONFIG_NVT_HDAL_KDRV_GFX2D),y)
obj-$(CONFIG_NVT_HDAL_KDRV_GFX2D) = $(MODULE_NAME).o
else
obj-m += $(MODULE_NAME).o
endif

OBJ := \
	kdrv_gfx2d_main.o \
	kdrv_grph/graphic_main.o \
	kdrv_grph/graphic_proc.o \
	kdrv_grph/graphic_drv.o \
	kdrv_grph/graphic_api.o \
	kdrv_grph/grph.o \
	kdrv_grph/grph_jobs.o \
	kdrv_grph/grph_platform.o \
	kdrv_grph/emu_grph.o \
	kdrv_grph/kdrv_graphic.o \
	kdrv_ise/eng/ise_eng_platform.o \
	kdrv_ise/eng/ise_eng.o \
	kdrv_ise/kdrv_ise_main.o \
	kdrv_ise/kdrv_ise_proc.o \
	kdrv_ise/kdrv_ise_drv.o \
	kdrv_ise/kdrv_ise_api.o \
	kdrv_ise/kdrv_ise_dbg.o \
	kdrv_ise/kdrv_ise_int.o \
	kdrv_ise/kdrv_ise.o \
	kdrv_ise/kdrv_ise_export.o \
	kdrv_ise/kdrv_ise_platform.o \
	kdrv_gfx_if/kdrv_gfx_if.o \
	kdrv_gfx_if/kdrv_gfx_if_export.o \

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
	kdrv_gfx2d_main.c \
	kdrv_grph/grph.c \
	kdrv_grph/grph_jobs.c \
	kdrv_grph/grph_platform.c \
	kdrv_grph/emu_grph.c \
	kdrv_grph/kdrv_graphic.c \
	kdrv_ise/eng/ise_eng_platform.c \
	kdrv_ise/eng/ise_eng.c \
	kdrv_ise/kdrv_ise_api.c \
	kdrv_ise/kdrv_ise_dbg.c \
	kdrv_ise/kdrv_ise_int.c \
	kdrv_ise/kdrv_ise.c \
	kdrv_ise/kdrv_ise_platform.c \
	kdrv_gfx_if/kdrv_gfx_if.o


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
