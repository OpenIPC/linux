MODULE_NAME = kdrv_comm
# DIRs
VOS_DRIVER_DIR = $(NVT_VOS_DIR)/drivers
KDRV_DIR = $(NVT_HDAL_DIR)/drivers/k_driver
CURR_DRIVER_DIR = $(KDRV_DIR)
RTOS_OUTPUT_DIR = $(CURR_DRIVER_DIR)/output
EXTRA_INCLUDE += \
	-I$(VOS_DRIVER_DIR)/include \
	-I$(KDRV_DIR)/include \
	-I$(KDRV_DIR)/include/plat \
	-I$(CURR_DRIVER_DIR)/source/include \
	-I$(CURR_DRIVER_DIR)/source/comm/ddr_arb/include \
	-I$(CURR_DRIVER_DIR)/source/comm/drv_sys/include \
	-I$(CURR_DRIVER_DIR)/source/comm/pwm/include \
	-I$(CURR_DRIVER_DIR)/source/comm/sif/include \
	-I$(KDRV_DIR)/include/comm \

.PHONY: modules modules_install clean
###############################################################################
# Linux Makefile                                                              #
###############################################################################
ifeq ($(NVT_PRJCFG_CFG),Linux)
C_CFLAGS  += -D_ARCH_ARM_=1 -D_ARCH_MIPS_=0 -D_ARCH_=$(_ARCH_ARM_)
C_CFLAGS  += -D__LINUX -Werror
EXTRA_CFLAGS += $(C_CFLAGS) $(EXTRA_INCLUDE) -Wno-date-time -Wno-sign-compare -D__SOC_680_PLATFORM__ -DDEBUG -D_GROUP_KO_
KBUILD_EXTRA_SYMBOLS = $(shell find $(NVT_HDAL_DIR)/drivers -name Module.symvers)
ccflags-y  := $(EXTRA_CFLAGS)

ifeq ($(CONFIG_NVT_HDAL_KDRV_COMM),y)
obj-$(CONFIG_NVT_HDAL_KDRV_COMM) = $(MODULE_NAME).o
else
obj-m += $(MODULE_NAME).o
endif

OBJ := \
	kdrv_comm_main.o \
	drv_sys/timer_main.o \
	drv_sys/timer_proc.o \
	drv_sys/timer_drv.o \
	drv_sys/timer_api.o \
	drv_sys/timer_platform.o \
	drv_sys/timer.o \
	drv_sys/timer_ioctl.o \
	drv_sys/timer_export.o \
	drv_sys/hwclock.o \
	drv_sys/heavyload.o \
	drv_sys/heavyload_proc.o \
	ddr_arb/ddr_arb_main.o \
	ddr_arb/ddr_arb_proc.o \
	ddr_arb/ddr_arb_drv.o \
	ddr_arb/ddr_arb_api.o \
	ddr_arb/ddr_arb_platform.o \
	ddr_arb/arb.o \
	ddr_arb/emu_arb.o \
	ddr_arb/arb_ver.o \
	ddr_arb/ddr_arb_export.o \
	pwm/pwm_main.o \
	pwm/pwm_proc.o \
	pwm/pwm_drv.o \
	pwm/pwm_api.o \
	pwm/pwm_platform.o \
	pwm/pwm.o \
	pwm/pwm_ver.o \
	pwm/pwm_export.o \
	kdrv_log/vg_log_pif.o \
	kdrv_log/vg_log_core.o \
	kdrv_log/vg_log_linux.o \
	kdrv_log/vg_log_pub.o \
	sif/sif_main.o \
	sif/sif_proc.o \
	sif/sif_drv.o \
	sif/sif_api.o \
	sif/sif.o \
	sif/sif_export.o \
	fdt/fdt.o \
	fdt/fdt_ro.o \
	fdt/fdt_wip.o \
	fdt/fdt_sw.o \
	fdt/fdt_rw.o \
	fdt/fdt_strerror.o \
	fdt/fdt_empty_tree.o \
	fdt/fdt_addresses.o \
	fdt/fdt_overlay.o \
	fdt/fdt_export.o \
	ddr_arb/arb_protected.o

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
include $(NVT_PRJCFG_MODEL_CFG)
OUTPUT_NAME = lib$(MODULE_NAME).a
EXTRA_INCLUDE += -I$(KDRV_DIR)/include/plat
C_CFLAGS = $(PLATFORM_CFLAGS) $(EXTRA_INCLUDE) -Wno-sign-compare -DDEBUG -D_$(CPU2_TYPE)_

SRC = \
	kdrv_comm_main.c \
	drv_sys/dummy_core.c \
	drv_sys/timer_platform.c \
	drv_sys/timer.c \
	drv_sys/timer_drv.c \
	drv_sys/hwclock.c \
	drv_sys/hwclock_freertos.c \
	drv_sys/delay.c \
	drv_sys/heavyload.o \
	ddr_arb/arb.c\
	ddr_arb/ddr_arb_platform.c\
	ddr_arb/arb_ver.c \
	pwm/pwm.c \
	pwm/pwm_platform.c \
	pwm/pwm_ver.c \
	kdrv_log/vg_log_pif.c \
	kdrv_log/vg_log_core.c \
	kdrv_log/vg_log_pub.c \
	kdrv_log/vg_log_rtos.c \
	fdt/fdt.c \
	fdt/fdt_ro.c \
	fdt/fdt_wip.c \
	fdt/fdt_sw.c \
	fdt/fdt_rw.c \
	fdt/fdt_strerror.c \
	fdt/fdt_empty_tree.c \
	fdt/fdt_addresses.c \
	fdt/fdt_overlay.c \
	drvdump/nvt_drvdump.c

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
