MODULE_NAME = kwrap
# DIRs
VOS_DRIVER_DIR = $(NVT_VOS_DIR)/drivers
RTOS_OUTPUT_DIR = $(VOS_DRIVER_DIR)/output
# INCs
VOS_INC_PATH = $(VOS_DRIVER_DIR)/include
VOS_SRC_INC_PATH = $(VOS_DRIVER_DIR)/source/include

# INC FLAGs
EXTRA_INCLUDE += -I$(VOS_SRC_INC_PATH)
.PHONY: modules modules_install clean
###############################################################################
# Linux Makefile                                                              #
###############################################################################
ifeq ($(NVT_PRJCFG_CFG),Linux)
C_CFLAGS  += -D_ARCH_ARM_=0 -D_ARCH_MIPS_=1 -D_ARCH_=$(_ARCH_ARM_)
C_CFLAGS  += -D__LINUX -Werror
ifeq ($(CONFIG_NVT_VOS_KWRAP),y)
C_CFLAGS += -D__NVT_VOS_KWRAP_USE_INITCALL
endif


EXTRA_CFLAGS += $(C_CFLAGS) $(EXTRA_INCLUDE) -I$(VOS_INC_PATH) -Wno-date-time -D__SOC_680_PLATFORM__ -I$(src)/include -DDEBUG
KBUILD_EXTRA_SYMBOLS = $(shell find $(VOS_DRIVER_DIR) -name Module.symvers)

ifeq ($(CONFIG_NVT_VOS_KWRAP),y)
obj-$(CONFIG_NVT_VOS_KWRAP) = $(MODULE_NAME).o
else
obj-m += $(MODULE_NAME).o
endif

OBJ := linux/rtos_os_dev.o
OBJ += linux/rtos_os_cpu.o
OBJ += linux/rtos_os_init.o
OBJ += linux/rtos_os_file.o
OBJ += linux/rtos_os_flag.o
OBJ += linux/rtos_os_mem.o
OBJ += linux/rtos_os_perf.o
OBJ += linux/rtos_os_proc.o
OBJ += linux/rtos_os_task.o
OBJ += linux/rtos_os_sem.o
OBJ += linux/rtos_os_spinlock.o
OBJ += linux/rtos_os_util.o
OBJ += linux/vos_ioctl.o
OBJ += linux/test_vos.o
OBJ += linux/sscanf.o
OBJ += linux/debug.o

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

endif
###############################################################################
# rtos Makefile                                                               #
###############################################################################
else ifeq ($(NVT_PRJCFG_CFG),rtos)
OUTPUT_NAME = lib$(MODULE_NAME).a

RTOS_KERNEL_DIR = $(KERNELDIR)/lib/FreeRTOS
RTOS_LIB_DIR = $(KERNELDIR)/lib
RTOS_CURR_DEMO_DIR = $(KERNELDIR)/demos/novatek/$(SDK_CODENAME)

EXTRA_INCLUDE += \
	-I$(RTOS_LIB_DIR) \
	-I$(RTOS_KERNEL_DIR)/portable/GCC/ARM_CA9  \
	-I$(RTOS_KERNEL_DIR)/include \
	-I$(RTOS_KERNEL_DIR)/include/private \
	-I$(RTOS_CURR_DEMO_DIR)/include \
	-I$(VOS_INC_PATH)

C_CFLAGS = $(PLATFORM_CFLAGS) $(EXTRA_INCLUDE) -DDEBUG

SRC = \
	freertos/debug.c \
	freertos/rtos_os_cpu.c \
	freertos/rtos_os_cpuwp.c \
	freertos/rtos_os_file.c \
	freertos/rtos_os_flag.c \
	freertos/rtos_os_mailbox.c \
	freertos/rtos_os_mem.c \
	freertos/rtos_os_perf.c \
	freertos/rtos_os_sem.c \
	freertos/rtos_os_spinlock.c \
	freertos/rtos_os_task.c \
	freertos/rtos_os_util.c \
	freertos/cmdsys.c \
	freertos/sscanf.c \
	freertos/verinfo.c \
	freertos/rtos_os_dumpstack.c \
	freertos/rtos_os_debug_cmd.c \
	freertos/examsys.c \


copy = if [ ! -z "$(1)" -a "$(1)" != " " ]; then cp -avf $(1) $(2); fi

OBJ = $(SRC:.c=.o)

ifeq ("$(wildcard *.c */*.c */*/*.c)","")
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
	@mkdir -p $(RTOS_OUTPUT_DIR)
	@cp -avf *.a $(RTOS_OUTPUT_DIR)

endif
