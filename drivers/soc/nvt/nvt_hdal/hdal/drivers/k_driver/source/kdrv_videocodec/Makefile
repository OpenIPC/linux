MODULE_NAME = kdrv_h26x
# DEFINE
VSIM = no
VEMU = no
# DIRs
VOS_DRIVER_DIR = $(NVT_VOS_DIR)/drivers
KDRV_DIR = $(NVT_HDAL_DIR)/drivers/k_driver
CURR_DRIVER_DIR = $(KDRV_DIR)
RTOS_OUTPUT_DIR = $(CURR_DRIVER_DIR)/output
# INCs
VOS_INC_PATH = $(VOS_DRIVER_DIR)/include
KDRV_INC_PATH = $(KDRV_DIR)/include
CURR_DRIVER_INC_PATH = $(CURR_DRIVER_DIR)/include
# INC FLAGs
EXTRA_INCLUDE += -I$(VOS_INC_PATH) -I$(KDRV_INC_PATH) -I$(CURR_DRIVER_INC_PATH)
EXTRA_INCLUDE += -I$(VOS_DRIVER_DIR)/include/ -I$(CURR_DRIVER_DIR)/include/ -I$(CURR_DRIVER_DIR)/source/kdrv_videocodec -I$(CURR_DRIVER_DIR)/source/include/
EXTRA_INCLUDE += -I$(LIBRARY_DIR)/include
EXTRA_INCLUDE += -I$(CURR_DRIVER_DIR)/source/kdrv_builtin/nvt_vdocdc/

.PHONY: modules modules_install clean
ifeq ($(VEMU),no)
vpu_dir = $(CURR_DRIVER_DIR)/source/kdrv_builtin/nvt_vdocdc/vpu
vrc_dir = $(CURR_DRIVER_DIR)/source/kdrv_builtin/nvt_vdocdc/vrc
h26x_dir = $(CURR_DRIVER_DIR)/source/kdrv_builtin/nvt_vdocdc/h26x
h264e_dir = $(CURR_DRIVER_DIR)/source/kdrv_builtin/nvt_vdocdc/h264
h264d_dir = $(CURR_DRIVER_DIR)/source/kdrv_videocodec/h264
h265e_dir = $(CURR_DRIVER_DIR)/source/kdrv_builtin/nvt_vdocdc/h265
h265d_dir = $(CURR_DRIVER_DIR)/source/kdrv_videocodec/h265
sim_dir = $(CURR_DRIVER_DIR)/source/kdrv_videocodec/sim
emode_dir = $(CURR_DRIVER_DIR)/source/kdrv_videocodec/emode
EXTRA_INCLUDE += -I$(vpu_dir)/inc -I$(vrc_dir)/inc -I$(h26x_dir)/inc -I$(h26x_dir)/api -I$(h264e_dir)/inc -I$(h264e_dir)/api -I$(h265e_dir)/inc -I$(h265e_dir)/api
EXTRA_INCLUDE += -I$(h264d_dir)/inc -I$(h264d_dir)/api -I$(h265d_dir)/inc -I$(h265d_dir)/api -I$(sim_dir)/inc -I$(emode_dir)/inc -I./include/ -I$(NVT_HDAL_DIR)/vendor/media/drivers/include/nvt_vencrc
else
vpu_dir = $(CURR_DRIVER_DIR)/source/kdrv_videocodec/vpu
vrc_dir = $(CURR_DRIVER_DIR)/source/kdrv_videocodec/vrc
h26x_dir = $(CURR_DRIVER_DIR)/source/kdrv_videocodec/h26x_emu
h264_dir = $(CURR_DRIVER_DIR)/source/kdrv_videocodec/h264_emu
h265_dir = $(CURR_DRIVER_DIR)/source/kdrv_videocodec/h265_emu
emu_dir = $(CURR_DRIVER_DIR)/source/kdrv_videocodec/emu
EXTRA_INCLUDE += -I$(NVT_HDAL_DIR)/vendor/media/drivers/include/nvt_vencrc -I$(vpu_dir)/inc -I$(vrc_dir)/inc -I$(h26x_dir)/inc -I$(h26x_dir)/api -I$(h264_dir)/inc -I$(h264_dir)/api -I$(h265_dir)/inc -I$(h265_dir)/api -I$(emu_dir)/inc
EXTRA_INCLUDE += -I$(KDRV_DIR)/include/plat
endif
###############################################################################
# Linux Makefile                                                              #
###############################################################################
ifeq ($(NVT_PRJCFG_CFG),Linux)
C_CFLAGS  += -D_ARCH_ARM_=1 -D_ARCH_MIPS_=0 -D_ARCH_=$(_ARCH_ARM_)
C_CFLAGS  += -D__LINUX -Werror
#C_CFLAGS  += -DVDOCDC_LL

ifeq ($(VSIM),yes)
C_CFLAGS += -DVDOCDC_SIM
sim-objs := sim/src/sim_vdocdc_mem.o sim/src/sim_vdocdc_file.o sim/src/sim_avc_main.o sim/src/sim_hevc_main.o
sim-objs += sim_jpeg_main.o
endif

EXTRA_CFLAGS += $(C_CFLAGS) $(EXTRA_INCLUDE) -Wno-date-time  -I$(src)/include -DDEBUG
KBUILD_EXTRA_SYMBOLS = $(shell find $(VOS_DRIVER_DIR) -name Module.symvers) $(shell find $(KDRV_DIR) -name Module.symvers)

h264-objs += h264/src/h264dec_api.o h264/src/h264dec_header.o h264/src/h264dec_int.o h264/src/h264dec_cfg.o
h265-objs += h265/src/h265dec_api.o h265/src/h265dec_header.o h265/src/h265dec_int.o h265/src/h265dec_cfg.o
emode-objs := emode/src/vemd_cfg.o emode/src/vemd_api.o

obj-m += $(MODULE_NAME).o
OBJ := kdrv_vdocdc_main.o kdrv_vdocdc_dbg.o kdrv_vdocdc_ioctl.o kdrv_vdocdc_emode.o kdrv_videoenc.o kdrv_videodec.o kdrv_vdocdc_proc.o kdrv_vdocdc_api.o kdrv_vdocdc_thread.o kdrv_vdocdc_export.o
OBJ += $(h264-objs) $(h265-objs) $(sim-objs) $(emode-objs)

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

C_CFLAGS = $(PLATFORM_CFLAGS) $(EXTRA_INCLUDE)

ifeq ($(VEMU),no)
SRC += $(wildcard $(h264d_dir)/src/*.c)
SRC += $(wildcard $(h265d_dir)/src/*.c)
SRC += $(wildcard $(emode_dir)/src/*.c)
SRC += kdrv_videoenc.c kdrv_videodec.c kdrv_vdocdc_dbg.c kdrv_vdocdc_emode.c
C_CFLAGS += -DDEBUG

ifeq ($(VSIM),yes)
C_CFLAGS += -DVDOCDC_SIM
SRC += $(wildcard $(sim_dir)/src/*.c)
endif
else
C_CFLAGS += -DVDOCDC_EMU
SRC += $(wildcard $(vpu_dir)/src/*.c)
SRC += $(wildcard $(h26x_dir)/src/*.c)
SRC += $(wildcard $(h264_dir)/src/*.c)
SRC += $(wildcard $(emu_dir)/src/*.c)
SRC += kdrv_vdocdc_dbg.c kdrv_vdocdc_emode.c
C_CFLAGS += -I$(APP_DIR)/source/test/emulation
C_CFLAGS += -DDEBUG
endif
copy = if [ ! -z "$(1)" -a "$(1)" != " " ]; then cp -avf $(1) $(2); fi

OBJ = $(patsubst %.c, %.o, $(SRC))

ifeq ("$(wildcard *.c */*.c */*/*.c)","")
modules:
	@echo "nothing to be done for '$(MODULE_NAME)'"

clean:
	@echo "nothing to be done for '$(MODULE_NAME)'"

modules_install:
	@echo "nothing to be done for '$(MODULE_NAME)'"
else

modules : $(OUTPUT_NAME)

%.o:%.c
	@echo Compiling $(notdir $<)
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
