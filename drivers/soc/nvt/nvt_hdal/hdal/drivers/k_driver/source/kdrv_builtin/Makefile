MODULE_NAME = kdrv_builtin
# DIRs
VOS_DRIVER_DIR = $(NVT_VOS_DIR)/drivers
KDRV_DIR = $(NVT_HDAL_DIR)/drivers/k_driver
CURR_DRIVER_DIR = $(KDRV_DIR)
RTOS_OUTPUT_DIR = $(CURR_DRIVER_DIR)/output
EXTRA_INCLUDE += \
	-I$(NVT_HDAL_DIR)/include \
	-I$(VOS_DRIVER_DIR)/include \
	-I$(KDRV_DIR)/include \
	-I$(KDRV_DIR)/include/comm \
	-I$(KDRV_DIR)/include/kdrv_builtin \
	-I$(KDRV_DIR)/source/include \
	-I$(KDRV_DIR)/source/kdrv_videoprocess/kdrv_ime/eng/include \
	-I$(CURR_DRIVER_DIR)/source/kdrv_builtin \
	-I$(CURR_DRIVER_DIR)/source/kdrv_builtin/ai \
	-I$(CURR_DRIVER_DIR)/source/kdrv_builtin/ipp/include \
	-I$(CURR_DRIVER_DIR)/source/kdrv_builtin/isp \
	-I$(CURR_DRIVER_DIR)/source/kdrv_builtin/include \
	-I$(CURR_DRIVER_DIR)/source/kdrv_builtin/sie \
	-I$(CURR_DRIVER_DIR)/source/kdrv_builtin/nvt_vdocdc/ \
	-I$(CURR_DRIVER_DIR)/source/kdrv_builtin/nvt_vdocdc/vpu/inc \
	-I$(CURR_DRIVER_DIR)/source/kdrv_builtin/nvt_vdocdc/vrc/inc \
	-I$(CURR_DRIVER_DIR)/source/kdrv_builtin/nvt_vdocdc/h26x/inc \
	-I$(CURR_DRIVER_DIR)/source/kdrv_builtin/nvt_vdocdc/h26x/api \
	-I$(CURR_DRIVER_DIR)/source/kdrv_builtin/nvt_vdocdc/h264/inc \
	-I$(CURR_DRIVER_DIR)/source/kdrv_builtin/nvt_vdocdc/h264/api \
	-I$(CURR_DRIVER_DIR)/source/kdrv_builtin/nvt_vdocdc/h265/inc \
	-I$(CURR_DRIVER_DIR)/source/kdrv_builtin/nvt_vdocdc/h265/api \
	-I$(CURR_DRIVER_DIR)/source/kdrv_builtin/audio/dai/include \
	-I$(CURR_DRIVER_DIR)/source/kdrv_builtin/audio/eac/include \
	-I$(NVT_HDAL_DIR)/vendor/media/drivers/include/nvt_vencrc \
	-I$(CURR_DRIVER_DIR)/source/kdrv_builtin/nvt_jpg/include \
	-I$(KDRV_DIR)/include/kdrv_ai \
	-I$(CURR_DRIVER_DIR)/source/kdrv_builtin/kdrv_ai/include \
	-I$(CURR_DRIVER_DIR)/source/kdrv_builtin/kdrv_ai/cnn \
	-I$(CURR_DRIVER_DIR)/source/kdrv_builtin/kdrv_ai/nue \
	-I$(CURR_DRIVER_DIR)/source/kdrv_builtin/kdrv_ai/nue2 \

.PHONY: modules modules_install clean
###############################################################################
# Linux Makefile                                                              #
###############################################################################
ifeq ($(NVT_PRJCFG_CFG),Linux)
C_CFLAGS  += -D_ARCH_ARM_=1 -D_ARCH_MIPS_=0 -D_ARCH_=$(_ARCH_ARM_)
C_CFLAGS  += -D__LINUX -Werror
ifeq ($(CONFIG_NVT_HDAL_KDRV_BUILTIN),y)
C_CFLAGS += -D__KDRV_BUILTIN_USE_INITCALL
endif
EXTRA_CFLAGS += $(C_CFLAGS) $(EXTRA_INCLUDE) -Wno-date-time -Wno-sign-compare -D_GROUP_KO_
KBUILD_EXTRA_SYMBOLS = $(shell find $(NVT_HDAL_DIR)/drivers -name Module.symvers)
ccflags-y  := $(EXTRA_CFLAGS)

ifeq ($(CONFIG_NVT_HDAL_KDRV_BUILTIN),y)
obj-$(CONFIG_NVT_HDAL_KDRV_BUILTIN) = $(MODULE_NAME).o
else
obj-m += $(MODULE_NAME).o
endif

vpu-objs := nvt_vdocdc/vpu/src/h26x.o
vrc-objs := nvt_vdocdc/vrc/src/h26xenc_rc.o
h26x-objs := nvt_vdocdc/h26x/src/h26x_bitstream.o nvt_vdocdc/h26x/src/h26x_common.o nvt_vdocdc/h26x/src/h26xenc_api.o nvt_vdocdc/h26x/src/h26xenc_int.o nvt_vdocdc/h26x/src/h26xenc_wrap.o
h264-objs := nvt_vdocdc/h264/src/h264enc_api.o nvt_vdocdc/h264/src/h264enc_header.o nvt_vdocdc/h264/src/h264enc_int.o nvt_vdocdc/h264/src/h264enc_wrap.o
h265-objs := nvt_vdocdc/h265/src/h265enc_api.o nvt_vdocdc/h265/src/h265enc_header.o nvt_vdocdc/h265/src/h265enc_int.o nvt_vdocdc/h265/src/h265enc_wrap.o
h26xm-objs := nvt_vdocdc/nvt_vdocdc_main.o nvt_vdocdc/nvt_vdocdc_drv.o nvt_vdocdc/nvt_vdocdc_dbg.o nvt_vdocdc/nvt_vdocdc_export.o
jpeg-objs := nvt_jpg/nvt_jpeg_main.o nvt_jpg/nvt_jpeg_proc.o nvt_jpg/nvt_jpeg_drv.o nvt_jpg/nvt_jpeg_api.o nvt_jpg/nvt_jpeg_platform.o nvt_jpg/nvt_jpeg.o nvt_jpg/nvt_jpeg_queue.o nvt_jpg/nvt_jpg_enc.o nvt_jpg/nvt_jpg_header.o nvt_jpg/nvt_jpeg_int.o nvt_jpg/nvt_jpegbrc.o nvt_jpg/nvt_jpg_parseheader.o
kdrv_ai-objs := kdrv_ai/ai_main.o kdrv_ai/ai_drv.o kdrv_ai/ai_lib.o kdrv_ai/kdrv_ai_api.o kdrv_ai/kdrv_ai_config.o kdrv_ai/kdrv_ai_int.o kdrv_ai/cnn/cnn_int.o kdrv_ai/cnn/cnn_lib.o kdrv_ai/cnn/cnn_platform.o kdrv_ai/nue/nue_int.o kdrv_ai/nue/nue_lib.o kdrv_ai/nue/nue_platform.o kdrv_ai/nue2/nue2_int.o kdrv_ai/nue2/nue2_lib.o kdrv_ai/nue2/nue2_platform.o

ifeq ($(shell if [ -d $(src)/isp/builtin ]; then echo y; else echo n; fi),y)
isp-builtin-dir := isp/builtin
else
isp-builtin-dir := isp
endif

isp-builtin-objs := \
	$(isp-builtin-dir)/ae_builtin_nvt.o \
	$(isp-builtin-dir)/ae_builtin_alg.o \
	$(isp-builtin-dir)/ae_common.o \
	$(isp-builtin-dir)/ae_dtsi.o \
	$(isp-builtin-dir)/ae_param.o \
	$(isp-builtin-dir)/awb_builtin_nvt.o \
	$(isp-builtin-dir)/awb_builtin_alg.o \
	$(isp-builtin-dir)/awb_dtsi.o \
	$(isp-builtin-dir)/awb_param.o \
	$(isp-builtin-dir)/iq_lib_nvt.o

OBJ := \
	kdrv_builtin_main.o \
	kdrv_builtin_util.o \
	kdrv_builtin_ver.o \
	nvtmpp_init.o \
	bridge.o \
	ipp/ime_builtin_main.o \
	ipp/ime_builtin.o \
	ipp/kdrv_ipp_builtin.o \
	ipp/kdrv_ipp_builtin_platform.o \
	ipp/kdrv_ipp_builtin_dbg.o \
	isp/ae_builtin.o \
	isp/awb_builtin.o \
	isp/iq_builtin.o \
	isp/iq_dtsi.o \
	isp/iq_param.o \
	isp/isp_builtin.o \
	isp/sensor_builtin.o \
	sie/sie_main.o \
	sie/sie_drv.o \
	sie/sie_int.o \
	sie/sie_lib.o \
	sie/sie_platform.o \
	sie/sie_init.o \
	sie/sie_init_platform.o \
	sie/sie_export.o \
	vdoenc_builtin.o \
	vdoenc_builtin_export.o \
	audio/dai/dai.o \
	audio/dai/dai_api.o \
	audio/dai/dai_main.o \
	audio/dai/dai_drv.o \
	audio/dai/dai_proc.o \
	audio/dai/dai_platform.o \
	audio/dai/dai_export.o \
	audio/eac/eac.o \
	audio/eac/eac_api.o \
	audio/eac/eac_main.o \
	audio/eac/eac_drv.o \
	audio/eac/eac_proc.o \
	audio/eac/eac_platform.o \
	audio/eac/eac_export.o \
	audcap_builtin.o \
	audcap_builtin_platform.o \
	audcap_builtin_export.o \
	kflow_ai_builtin.o \
	kflow_ai_builtin_export.o \
	ai/pdcnn_api.o \
	ai/pdcnn_postproc.o \
	unlzma.o \

OBJ += $(vpu-objs)
OBJ += $(vrc-objs)
OBJ += $(h26x-objs)
OBJ += $(h264-objs)
OBJ += $(h265-objs)
OBJ += $(h26xm-objs)
OBJ += $(jpeg-objs)
OBJ += $(isp-builtin-objs)
OBJ += $(kdrv_ai-objs)

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
EXTRA_INCLUDE += \
	-I$(KDRV_DIR)/include/plat \
	-I$(KDRV_DIR)/include \

C_CFLAGS = $(PLATFORM_CFLAGS) $(EXTRA_INCLUDE) -Wno-sign-compare -DDEBUG

SRC = \
	kdrv_builtin_main.c \
	kdrv_builtin_ver.c \
	ipp/ime_builtin.c \
	ipp/kdrv_ipp_builtin.c \
	ipp/kdrv_ipp_builtin_platform.c \
	isp/ae_builtin.c \
	isp/awb_builtin.c \
	isp/iq_builtin.c \
	isp/iq_dtsi.c \
	isp/iq_param.c \
	isp/isp_builtin.c \
	isp/sensor_builtin.c \
	sie/sie_int.c \
	sie/sie_lib.c \
	sie/sie_platform.c \
	audio/dai/dai.c \
	audio/eac/eac.c \
	unlzma.c \
	kdrv_ai/ai_api.c \
	kdrv_ai/ai_lib.c \
	kdrv_ai/kdrv_ai_int.c \
	kdrv_ai/kdrv_ai_config.c \
	kdrv_ai/kdrv_ai_api.c \
	kdrv_ai/ai_ioctl.c \
	kdrv_ai/nue2/nue2_int.c \
	kdrv_ai/nue2/nue2_lib.c \
	kdrv_ai/nue2/nue2_platform.c \
	kdrv_ai/cnn/cnn_int.c \
	kdrv_ai/cnn/cnn_lib.c \
	kdrv_ai/cnn/cnn_platform.c \
	kdrv_ai/cnn/cnn_ll_cmd.c \
	kdrv_ai/nue/nue_ll_cmd.c \
	kdrv_ai/nue/nue_int.c \
	kdrv_ai/nue/nue_lib.c \
	kdrv_ai/nue/nue_platform.c \
	

SRC += $(wildcard nvt_vdocdc/vpu/src/*.c)
SRC += $(wildcard nvt_vdocdc/vrc/src/*.c)
SRC += $(wildcard nvt_vdocdc/h26x/src/*.c)
SRC += $(wildcard nvt_vdocdc/h264/src/*.c)
SRC += $(wildcard nvt_vdocdc/h265/src/*.c)
SRC += $(wildcard nvt_jpg/*.c)
SRC += nvt_vdocdc/nvt_vdocdc_drv.c nvt_vdocdc/nvt_vdocdc_dbg.c

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
