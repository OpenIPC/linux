MODULE_NAME = kflow_videoenc
# DIRs
VOS_DRIVER_DIR = $(NVT_VOS_DIR)/drivers
KDRV_DIR =$(NVT_HDAL_DIR)/drivers/k_driver
KFLOW_DIR =$(NVT_HDAL_DIR)/drivers/k_flow
CURR_DRIVER_DIR = $(KFLOW_DIR)
RTOS_OUTPUT_DIR = $(CURR_DRIVER_DIR)/output
# INCs
VOS_INC_PATH = $(VOS_DRIVER_DIR)/include
KDRV_INC_PATH = $(KDRV_DIR)/include
KDRV_INC_COMM_PATH = $(KDRV_DIR)/include/comm
KDRV_INC_PLAT_PATH = $(KDRV_DIR)/include/plat
KFLOW_INC_PATH = $(KFLOW_DIR)/include
CURR_DRIVER_INC_PATH = $(CURR_DRIVER_DIR)/include
# INC FLAGs
EXTRA_INCLUDE += -I$(VOS_INC_PATH) -I$(KDRV_INC_PATH) -I$(KFLOW_INC_PATH) -I$(CURR_DRIVER_INC_PATH) -I$(KDRV_INC_COMM_PATH)
.PHONY: modules modules_install clean
###############################################################################
# Linux Makefile                                                              #
###############################################################################
ifeq ($(NVT_PRJCFG_CFG),Linux)
C_CFLAGS  += -D__LINUX -Werror
#C_CFLAGS  += -DVDOENC_LL
EXTRA_INCLUDE += -I$(NVT_HDAL_DIR)/drivers/k_flow/source/kflow_videoenc/unit/include_protected

# This line should be removed later ( now still use dal_h26x API & nvt_h26x.ko first, will use kdrv API later )
EXTRA_INCLUDE += -I$(NVT_HDAL_DIR)/drivers/k_driver/source/kdrv_videocodec/include
# Need to include h265_def.h for tile rule
#EXTRA_INCLUDE += -I$(NVT_HDAL_DIR)/drivers/k_driver/source/kdrv_videocodec/h265/api
EXTRA_INCLUDE += -I$(NVT_HDAL_DIR)/drivers/k_driver/source/kdrv_builtin/nvt_vdocdc/h265/api

EXTRA_CFLAGS += $(C_CFLAGS) $(EXTRA_INCLUDE) -Wno-date-time -Wno-missing-braces  -I$(src)/include -DDEBUG
KBUILD_EXTRA_SYMBOLS = $(shell find $(VOS_DRIVER_DIR) -name Module.symvers) $(shell find $(KDRV_DIR) -name Module.symvers) $(shell find $(KFLOW_DIR) -name Module.symvers)
#ccflags-y  := $(EXTRA_CFLAGS)

obj-m += $(MODULE_NAME).o
OBJ := \
	linux/isf_vdoenc_main.o \
	linux/isf_vdoenc_proc.o \
	linux/isf_vdoenc_drv.o \
	mp_vcodec_mjpg/mp_mjpg_encoder.o \
	mp_vcodec_h264/mp_h264_encoder.o \
	mp_vcodec_h264/mp_h264_encoder_obj.o \
	mp_vcodec_h265/mp_h265_encoder.o \
	mp_vcodec_h265/mp_h265_encoder_obj.o \
	nmediarec_vdoenc/nvt_media_interface.o \
	nmediarec_vdoenc/nmediarec_vdoenc_tsk.o \
	nmediarec_vdoenc/nmediarec_vdotrig_tsk.o \
	nmediarec_vdoenc/nmediarec_vdoenc_platform.o \
	nmediarec_vdoenc/nmediarec_vdoenc_export.o \
	isf_vdoenc/isf_vdoenc.o \
	isf_vdoenc/isf_vdoenc_input_mask.o \
	isf_vdoenc/isf_vdoenc_input_osd.o \
	isf_vdoenc/isf_vdoenc_debug.o \
	isf_vdoenc/isf_vdoenc_verinfo.o \
	isf_vdoenc/isf_vdoenc_platform.o \

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
	@$(MAKE) -C $(KDIR) M=$(PWD) $(NVT_KGCOV) modules

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
C_CFLAGS = $(PLATFORM_CFLAGS) $(EXTRA_INCLUDE) -DDEBUG -Wno-format
EXTRA_INCLUDE += -I$(NVT_HDAL_DIR)/drivers/k_flow/source/kflow_videoenc/unit/include_protected
EXTRA_INCLUDE += -I$(NVT_HDAL_DIR)/drivers/k_driver/source/kdrv_videocodec/include
# Need to include h265_def.h for tile rule
#EXTRA_INCLUDE += -I$(NVT_HDAL_DIR)/drivers/k_driver/source/kdrv_videocodec/h265/api
EXTRA_INCLUDE += -I$(NVT_HDAL_DIR)/drivers/k_driver/source/kdrv_builtin/nvt_vdocdc/h265/api

ifeq ($(RTOS_CPU_TYPE), cortex-a9)
# cortex-a9 = NA51055
SRC = \
	rtos/isf_vdoenc_drv.o \
	rtos/isf_vdoenc_proc.o \
	mp_vcodec_mjpg/mp_mjpg_encoder.o \
	mp_vcodec_h264/mp_h264_encoder.o \
	mp_vcodec_h264/mp_h264_encoder_obj.o \
	mp_vcodec_h265/mp_h265_encoder.o \
	mp_vcodec_h265/mp_h265_encoder_obj.o \
	nmediarec_vdoenc/nvt_media_interface.o \
	nmediarec_vdoenc/nmediarec_vdoenc_tsk.o \
	nmediarec_vdoenc/nmediarec_vdotrig_tsk.o \
	nmediarec_vdoenc/nmediarec_vdoenc_platform.o \
	isf_vdoenc/isf_vdoenc.o \
	isf_vdoenc/isf_vdoenc_input_mask.o \
	isf_vdoenc/isf_vdoenc_input_osd.o \
	isf_vdoenc/isf_vdoenc_debug.o \
	isf_vdoenc/isf_vdoenc_verinfo.o \
	isf_vdoenc/isf_vdoenc_platform.o \

else
# cortex-a53 = NA51000
SRC = \
	dummy/dummy.o \

endif

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
