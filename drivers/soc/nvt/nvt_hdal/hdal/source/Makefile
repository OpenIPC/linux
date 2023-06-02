MODULE_NAME = libhdal
# DIRs
KDRV_DIR = $(NVT_HDAL_DIR)/drivers/k_driver
KFLOW_DIR = $(NVT_HDAL_DIR)/drivers/k_flow
OUTPUT_DIR = $(NVT_HDAL_DIR)/output
# INC FLAGs
EXTRA_INCLUDE += \
	-I$(NVT_VOS_DIR)/include \
	-I$(NVT_VOS_DIR)/drivers/include \
	-I$(KDRV_DIR)/include \
	-I$(KFLOW_DIR)/include \
	-I$(NVT_HDAL_DIR)/include \
	-I$(NVT_HDAL_DIR)/source/include \
	-I$(NVT_HDAL_DIR)/drivers/k_flow/source/kflow_videoenc/unit/include_protected \
	-I$(NVT_HDAL_DIR)/drivers/k_driver/source/kdrv_videocodec/include \
	-I$(NVT_HDAL_DIR)/drivers/k_flow/source/kflow_videodec/unit/include_protected \
	-I$(NVT_HDAL_DIR)/drivers/k_flow/source/kflow_audioenc/unit/include_protected \
	-I$(NVT_HDAL_DIR)/drivers/k_flow/source/kflow_audiodec/unit/include_protected

.PHONY: all clean
###############################################################################
# Linux Makefile                                                              #
###############################################################################
ifeq ($(NVT_PRJCFG_CFG),Linux)
#--------- ENVIRONMENT SETTING --------------------
WARNING	= -Wall -Wundef -Wsign-compare -Wno-missing-braces -Wstrict-prototypes -Werror
COMPILE_OPTS	= -I. -O2 -fPIC -ffunction-sections -fdata-sections -D__LINUX_USER__ -D__LINUX
C_CFLAGS	= $(NVT_GCOV) $(PLATFORM_CFLAGS) $(COMPILE_OPTS) $(WARNING) $(EXTRA_INCLUDE)
LD_FLAGS	= $(NVT_GCOV) -fPIC -shared -lpthread
#--------- END OF ENVIRONMENT SETTING -------------
SRC = hd_common.c hd_int.c hd_util.c \
	hd_audiocapture.c hd_audiodec.c hd_audioenc.c hd_audioout.c \
	hd_videocapture.c hd_videodec.c hd_videoenc.c hd_videoout.c hd_videoprocess.c \
	hd_osg.c hd_gfx.c

SRC += \
	hd_common_menu.c \
	hd_audiodec_menu.c hd_audioenc_menu.c \
	hd_videocapture_menu.c hd_videodec_menu.c hd_videoenc_menu.c hd_videoprocess_menu.c \
	hd_osg_menu.c hd_videoout_menu.c \
	hd_audiocapture_menu.c hd_audioout_menu.c \
	hd_util_menu.c

SRC += \
	hd_debug/hd_debug_get.c \
	hd_debug/hd_debug_set.c \
	hd_debug/hd_debug_menu.c \
	hd_logger/hd_logger.c \
	hd_logger/hd_logger_p.c

OBJ = $(SRC:.c=.o)

ifeq ("$(wildcard *.c */*.c)","")
all:
	@echo "nothing to be done for '$(OUTPUT_NAME)'"
clean:
	@echo "nothing to be done for '$(OUTPUT_NAME)'"
else
all: $(OBJ)
	@echo Creating library $(MODULE_NAME) ...
	@$(CC) -o $(MODULE_NAME).so $(OBJ) $(LD_FLAGS)
	@$(AR) rcs $(MODULE_NAME).a $(OBJ)

%.o:%.c
	@echo Compiling $<
	@$(CC) $(C_CFLAGS) $< -c -o $@

clean:
	@rm -f $(OBJ) *.o *.a *.so*
	@rm -rf $(OUTPUT_DIR)
endif

install:
	@mkdir -p $(OUTPUT_DIR)
	@cp -avf *.so* *.a $(OUTPUT_DIR)

###############################################################################
# rtos Makefile                                                               #
###############################################################################
else ifeq ($(NVT_PRJCFG_CFG),rtos)
#--------- ENVIRONMENT SETTING --------------------
# DIRs
RTOS_KERNEL_DIR = $(KERNELDIR)/lib/FreeRTOS
RTOS_LIB_DIR = $(KERNELDIR)/lib
RTOS_CURR_DEMO_DIR = $(KERNELDIR)/demos/novatek/na51089
RTOS_POSIX_DIR = $(KERNELDIR)/lib/FreeRTOS-Plus-POSIX
RTOS_POSIX_SRC_DIR = $(RTOS_POSIX_DIR)/source

#INCs for C_CFLAGS

EXTRA_INCLUDE += \
	-I$(RTOS_LIB_DIR) \
	-I$(RTOS_KERNEL_DIR)/portable/GCC/ARM_CA9  \
	-I$(RTOS_KERNEL_DIR)/include \
	-I$(RTOS_KERNEL_DIR)/include/private \
	-I$(RTOS_CURR_DEMO_DIR)/include \
	-I$(RTOS_POSIX_DIR)/include \
	-I$(RTOS_POSIX_DIR)/include/portable/novatek

C_CFLAGS = $(PLATFORM_CFLAGS) $(EXTRA_INCLUDE) -Wno-format
#--------- END OF ENVIRONMENT SETTING -------------
LIB_NAME = $(MODULE_NAME).a

SRC = hd_common.c \
	hd_int.c \
	hd_common_menu.c
SRC += \
	hd_util.c \
	hd_util_menu.c
SRC += \
	hd_audiocapture.c \
	hd_audiocapture_menu.c
SRC += \
	hd_audiodec.c \
	hd_audiodec_menu.c
SRC += \
	hd_audioenc.c \
	hd_audioenc_menu.c
SRC += \
	hd_audioout.c \
	hd_audioout_menu.c
 SRC += \
	hd_videocapture.c \
	hd_videocapture_menu.c
SRC += \
	hd_videodec.c \
	hd_videodec_menu.c
SRC += \
	hd_videoenc.c \
	hd_videoenc_menu.c
SRC += \
 	hd_videoout.c \
 	hd_videoout_menu.c
SRC += \
	hd_videoprocess.c \
	hd_videoprocess_menu.c
SRC += \
	hd_osg.c \
	hd_osg_menu.c
SRC += \
	hd_gfx.c
SRC += \
	hd_debug/hd_debug_get.c \
	hd_debug/hd_debug_set.c \
	hd_debug/hd_debug_menu.c \
	hd_logger/hd_logger.c \
	hd_logger/hd_logger_p.c



OBJ = $(SRC:.c=.o)

all: $(LIB_NAME)

$(LIB_NAME): $(OBJ)
	@echo Creating $@...
	@$(AR) rcsD $@ $(OBJ)
	@$(BUILD_DIR)/nvt-tools/nvt-ld-op --arc-sha1 $@

%.o: %.c
	@echo Compiling $<
	@$(CC) $(C_CFLAGS) -c $< -o $@

clean:
	@rm -f $(LIB_NAME) $(OBJ) $(LIB_NAME).sym *.o *.a *.so*
	@rm -rf $(OUTPUT_DIR)

install:
	@mkdir -p $(OUTPUT_DIR)
	@cp -avf $(LIB_NAME) $(OUTPUT_DIR)
endif
