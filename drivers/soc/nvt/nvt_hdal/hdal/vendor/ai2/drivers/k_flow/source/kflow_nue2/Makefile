NVT_KDRV_PATH=$(NVT_HDAL_DIR)/drivers/k_driver
NVT_KFLOW_PATH=$(NVT_HDAL_DIR)/drivers/k_flow
NVT_AI_KDRV_PATH=$(NVT_HDAL_DIR)/vendor/ai/drivers/k_driver
NVT_AI_KFLOW_PATH=$(NVT_HDAL_DIR)/vendor/ai2/drivers/k_flow

VG_MODULE_INC = $(NVT_KDRV_PATH)/source/include
VOS_MODULE_INC=$(NVT_VOS_DIR)/drivers/include

EXTRA_CFLAGS += -I$(VOS_MODULE_INC)
EXTRA_CFLAGS += -I$(NVT_HDAL_DIR)/include
EXTRA_CFLAGS += -I$(NVT_KDRV_PATH)/include -I$(VG_MODULE_INC) -I$(VG_MODULE_INC)/common -I$(VG_MODULE_INC)/util
EXTRA_CFLAGS += -I$(NVT_AI_KDRV_PATH)/include -I$(NVT_AI_KFLOW_PATH)/include
EXTRA_CFLAGS += -Werror -Wno-enum-compare -Wno-date-time -D__LINUX


MODULE_NAME = kflow_nue2
OBJ := kflow_ai_nue2.o 

obj-m += $(MODULE_NAME).o
obj-m := $(sort $(obj-m))

###############################################################################
# Linux Makefile                                                              #
###############################################################################
ifeq ($(NVT_PRJCFG_CFG),Linux)
# To build modules outside of the kernel tree, we run "make"
# in the kernel source tree; the Makefile these then includes this
# Makefile once again.
# This conditional selects whether we are being included from the
# kernel Makefile or not.
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
MODPATH := $(shell echo $(PWD) | awk -F'source/' '{print $$NF}')
MODNAME := $(shell echo $(obj-m:.o=.ko))
# variables for clean object
RM_TRACER_OBJ = $(foreach n, $(OBJ:.o=), $(if $(wildcard $(n).c),$(if $(wildcard $(n).o),$(n).o,),))
RM_NO_TRACER_OBJ = $(foreach n, $(OBJ:.o=), $(if $(wildcard $(n).c),$(if $(wildcard $(n)_no_tracer.o),$(n)_no_tracer.o,),))
export KBUILD_EXTRA_SYMBOLS += $(shell find $(NVT_KDRV_PATH) -name Module.symvers)
export KBUILD_EXTRA_SYMBOLS += $(shell find $(NVT_AI_KDRV_PATH) -name Module.symvers)

modules:
	$(MAKE) -C $(KDIR) M=$(PWD) $(NVT_KGCOV) modules

modules_install:
	@rm -f $(MDIR)/$(MODPATH)/$(MODNAME); \
	install -m644 -b -D $(MODNAME) ${MDIR}/$(MODPATH)/$(MODNAME); \
	cd $(KDIR)/_install_modules/lib/modules/$(KERVER)/; depmod -b $(KDIR)/_install_modules/ -a $(KERVER)
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
.PHONY: modules modules_install clean

endif
###############################################################################
# rtos Makefile                                                               #
###############################################################################
else ifeq ($(NVT_PRJCFG_CFG),rtos)
OUTPUT_NAME = lib$(MODULE_NAME).a
RTOS_OUTPUT_DIR = $(NVT_KDRV_PATH)/output
KDRV_DIR_OUTPUT = $(NVT_KDRV_PATH)/output

EXTRA_INCLUDE += -I$(NVT_KDRV_PATH)/include/rtos_na51089
EXTRA_INCLUDE += -I$(NVT_AI_KDRV_PATH)/include
EXTRA_INCLUDE += -I$(NVT_AI_KFLOW_PATH)/include
EXTRA_INCLUDE += -I$(VOS_MODULE_INC)
EXTRA_INCLUDE += -I$(NVT_HDAL_DIR)/include
EXTRA_INCLUDE += -I$(NVT_KDRV_PATH)/include
EXTRA_INCLUDE += -I$(NVT_KFLOW_PATH)/include
EXTRA_INCLUDE += -I$(VG_MODULE_INC)
EXTRA_INCLUDE += -I$(VG_MODULE_INC)/common
EXTRA_INCLUDE += -I$(VG_MODULE_INC)/util

C_CFLAGS = $(PLATFORM_CFLAGS) $(EXTRA_INCLUDE)
SRC = kflow_ai_nue2.o 

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
	@rm -f $(OBJ) *.o *.a *.so*
endif

modules_install:
	@mkdir -p $(RTOS_OUTPUT_DIR)
	@cp -avf *.a $(RTOS_OUTPUT_DIR)
	@cp -avf *.a $(KDRV_DIR_OUTPUT)
endif
