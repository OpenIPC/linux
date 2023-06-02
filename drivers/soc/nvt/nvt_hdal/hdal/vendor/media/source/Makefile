MODULE_NAME = libvendor_media
# DIRs
MEDIA_LIB_DIR = $(NVT_HDAL_DIR)/vendor/media
OUTPUT_DIR = $(NVT_HDAL_DIR)/vendor/output
KDRV_DIR = $(NVT_HDAL_DIR)/drivers/k_driver
KFLOW_DIR = $(NVT_HDAL_DIR)/drivers/k_flow
VOS_DRIVER_DIR = $(NVT_VOS_DIR)/drivers
# INCs
HDAL_INC_PATH = $(NVT_HDAL_DIR)/include
MEDIA_LIB_INC_PATH = $(MEDIA_LIB_DIR)/include
VOS_INC_PATH = $(VOS_DRIVER_DIR)/include
KDRV_INC_PATH = $(KDRV_DIR)/include
KFLOW_INC_PATH = $(KFLOW_DIR)/include
# INC FLAGs
EXTRA_INCLUDE += -I$(HDAL_INC_PATH) -I$(MEDIA_LIB_INC_PATH) -I$(VOS_INC_PATH) -I$(KDRV_INC_PATH) -I$(KFLOW_INC_PATH) -I$(KFLOW_DIR)/source/kflow_videodec/unit/include_protected
.PHONY: all clean
###############################################################################
# Linux Makefile                                                              #
###############################################################################
ifeq ($(NVT_PRJCFG_CFG),Linux)
#--------- ENVIRONMENT SETTING --------------------
WARNING	= -Wall -Wundef -Wsign-compare -Wno-missing-braces -Wstrict-prototypes -Werror
COMPILE_OPTS	= -I. -O2 -fPIC -ffunction-sections -fdata-sections -D__LINUX_USER__
CPPFLAGS	=
CFLAGS		= $(PLATFORM_CFLAGS) $(PRJCFG_CFLAGS)
C_CFLAGS	= $(NVT_GCOV) $(COMPILE_OPTS) $(CPPFLAGS) $(CFLAGS) $(WARNING) $(EXTRA_INCLUDE) -D__LINUX -Werror
LD_FLAGS	= $(NVT_GCOV) -fPIC -shared
#--------- END OF ENVIRONMENT SETTING -------------

#--------- Compiling --------------------
LIB_NAME=libvendor_media

SRC = vendor_audiocapture.c \
	  vendor_audiodec.c \
	  vendor_audioenc.c \
	  vendor_audioout.c \
	  vendor_videocapture.c \
	  vendor_videoprocess.c \
	  vendor_videoenc.c \
	  vendor_videodec.c \
	  vendor_videoout.c \
	  vendor_gfx.c \
	  vendor_common.c

copy = if [ ! -z "$(1)" -a "$(1)" != " " ]; then cp -avf $(1) $(2); fi

OBJ = $(SRC:.c=.o)

ifeq ("$(wildcard *.c */*.c */*/*.c)","")
all:
	@echo ">>> Skip"
clean:
	@echo ">>> Skip"
codesize:
	@echo ">>> Skip"
install_header:
	@echo ">>> Skip"
else
all : $(OBJ)
	@echo Creating library $(LIB_NAME).a ...
	@$(CC) -o $(LIB_NAME).so $(OBJ) $(LD_FLAGS)
	@$(AR) rcs $(LIB_NAME).a $(OBJ)

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
ISP_DRIVER_DIR = $(NVT_HDAL_DIR)/vendor/isp/drivers
# INCs
ISP_DRIVER_INC_PATH = $(ISP_DRIVER_DIR)/include
# INC FLAGs
EXTRA_INCLUDE += -I$(ISP_DRIVER_INC_PATH)
#--------- ENVIRONMENT SETTING --------------------
C_CFLAGS = $(PLATFORM_CFLAGS) $(EXTRA_INCLUDE) -Wno-format
#--------- END OF ENVIRONMENT SETTING -------------
LIB_NAME = $(MODULE_NAME).a
SRC = vendor_audiocapture.c \
	  vendor_audiodec.c \
	  vendor_audioenc.c \
	  vendor_audioout.c \
	  vendor_videocapture.c \
	  vendor_videoprocess.c \
	  vendor_videoenc.c \
	  vendor_videodec.c \
	  vendor_videoout.c \
	  vendor_gfx.c \
	  vendor_common.c
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
