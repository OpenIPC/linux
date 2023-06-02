MODULE_NAME = kdrv_ai
KDRV_DIR = $(NVT_HDAL_DIR)/drivers/k_driver
CURR_DRIVER_DIR = $(KDRV_DIR)
RTOS_OUTPUT_DIR = $(CURR_DRIVER_DIR)/output
NVT_KDRV_PATH=$(NVT_HDAL_DIR)/drivers/k_driver
NVT_AI_KDRV_PATH=$(NVT_HDAL_DIR)/vendor/ai/drivers/k_driver
NVT_AI_KFLOW_PATH=$(NVT_HDAL_DIR)/vendor/ai/drivers/k_flow
KDRV_INC_PATH = $(KDRV_DIR)/include
KDRV_DIR_OUTPUT = $(KDRV_DIR)/output
VG_MODULE_INC = $(NVT_KDRV_PATH)/source/include
VOS_DRIVER_DIR = $(NVT_VOS_DIR)/drivers
VOS_INC_PATH = $(VOS_DRIVER_DIR)/include

AI_EMUF_PATH = $(NVT_HDAL_DIR)/vendor/alg/ai/drivers/source

EXTRA_CFLAGS += -I$(NVT_HDAL_DIR)/include
EXTRA_CFLAGS += -I$(NVT_KDRV_PATH)/include -I$(VG_MODULE_INC) -I$(VG_MODULE_INC)/common -I$(VG_MODULE_INC)/util -I$(VOS_INC_PATH)
EXTRA_CFLAGS += -I$(NVT_KDRV_PATH)/include/kdrv_builtin
EXTRA_CFLAGS += -I$(NVT_AI_KDRV_PATH)/include
EXTRA_CFLAGS += -I$(NVT_AI_KFLOW_PATH)/include/net_flow_sample
EXTRA_CFLAGS += -I$(NVT_AI_KFLOW_PATH)/include/kflow_ai_net
#EXTRA_CFLAGS += -I$(NVT_AI_KDRV_PATH)/source/include
EXTRA_CFLAGS += -I$(NVT_AI_KDRV_PATH)/source/kdrv_ai/include
EXTRA_CFLAGS += -I$(NVT_AI_KDRV_PATH)/source/kdrv_ai/cnn
EXTRA_CFLAGS += -I$(NVT_AI_KDRV_PATH)/source/kdrv_ai/nue
EXTRA_CFLAGS += -I$(NVT_AI_KDRV_PATH)/source/kdrv_ai/nue2
EXTRA_CFLAGS += -I$(LINUX_BUILD_TOP)/linux-supplement/include

EXTRA_INCLUDE += \
				-I$(NVT_AI_KDRV_PATH)/source/kdrv_ai/nue \
				-I$(NVT_AI_KDRV_PATH)/source/kdrv_ai/nue2 \
				-I$(NVT_AI_KDRV_PATH)/source/kdrv_ai/cnn \
				-I$(CURR_DRIVER_DIR)/source/include \
				-I$(NVT_HDAL_DIR)/include \
				-I$(NVT_AI_KDRV_PATH)/include \
				-I$(KDRV_DIR)/include \
				-I$(VOS_DRIVER_DIR)/include \
				-I$(NVT_AI_KFLOW_PATH)/include/net_flow_sample \
				-I$(LIBRARY_DIR)/include \
				-I$(VOS_DRIVER_DIR)/include \
				-I$(KDRV_INC_PATH)\
				-I$(KDRV_INC_PATH)/comm \
				-I$(AI_EMUF_PATH) \
				-I../../../../../../../application/source/test/emulation \
				-I../../../../../../../../base/rtos-code/application/source/test/emulation \


ifeq ("${RTOS_CPU_TYPE}","cortex-a53")
EXTRA_INCLUDE += -I$(KDRV_INC_PATH)/rtos_na51000
EXTRA_INCLUDE += -I../../../../../../../driver/na51000/include
else
EXTRA_INCLUDE += -I$(KDRV_INC_PATH)/rtos_na51089
EXTRA_INCLUDE += -I../../../../../../../driver/na51089/include
EXTRA_INCLUDE += -I../../../../../../../../base/rtos-code/driver/na51089/include
EXTRA_INCLUDE += -I../../../../../../../hdal/vendor/ai2/drivers/k_flow/include/kflow_ai_net
endif

.PHONY: modules modules_install clean

###############################################################################
# Linux Makefile                                                              #
###############################################################################
ifeq ($(NVT_PRJCFG_CFG),Linux)

EXTRA_CFLAGS += -D__LINUX -Werror -Wno-enum-compare -Wno-date-time
EXTRA_CFLAGS += -D__section_name__=$(MODULE_NAME)

OBJ := ai_api.o ai_drv.o ai_main.o ai_proc.o ai_lib.o \
                  kdrv_ai_api.o kdrv_ai_config.o kdrv_ai_int.o \
                  cnn/cnn_ll_cmd.o cnn/cnn_int.o cnn/cnn_lib.o \
                  nue/nue_ll_cmd.o nue/nue_int.o nue/nue_lib.o \
				  nue2/nue2_ll_cmd.o nue2/nue2_int.o \
				  nue2/nue2_lib.o nue2/nue2_sys_vfy_emu.o \
                  nue2/nue2_platform.o nue/nue_platform.o cnn/cnn_platform.o \

obj-m += $(MODULE_NAME).o
obj-m := $(sort $(obj-m))


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
	cd $(KDIR)/_install_modules/lib/modules/$(KERVER)/; depmod -b $(KDIR)/_install_modules/ -a $(KERVER); \
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
C_CFLAGS = $(PLATFORM_CFLAGS) $(EXTRA_INCLUDE) -DDEBUG -I$(CURDIR)/include

SRC = \
		ai_api.c \
		ai_lib.c \
		kdrv_ai_int.c \
		kdrv_ai_config.c \
		kdrv_ai_api.c \
		ai_ioctl.c \
		nue2/nue2_int.c \
		nue2/nue2_lib.c \
        nue2/nue2_platform.c \
		nue2/nue2_ll_cmd.c \
		cnn/cnn_int.c \
		cnn/cnn_lib.c \
        cnn/cnn_platform.c \
		cnn/cnn_ll_cmd.c \
		nue/nue_ll_cmd.c \
		nue/nue_int.c \
		nue/nue_lib.c \
		nue/nue_platform.c \
#Please reference to ./hdal/drivers/k_driver/source/kdrv_gfx2d/kdrv_affine/affine_platform.c

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
	@rm -f $(OBJ) *.o *.a *.so*
endif

modules_install:
	@mkdir -p $(RTOS_OUTPUT_DIR)
	@cp -avf *.a $(RTOS_OUTPUT_DIR)
	@cp -avf *.a $(KDRV_DIR_OUTPUT)

endif


