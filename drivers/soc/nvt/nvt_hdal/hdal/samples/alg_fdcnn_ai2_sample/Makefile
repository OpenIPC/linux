MODULE_NAME = alg_fdcnn_ai2_sample
# DIRs
HDAL_SAMPLE_DIR = $(NVT_HDAL_DIR)/samples
ECOS_OUTPUT_DIR = $(HDAL_SAMPLE_DIR)/output
VOS_DRIVER_DIR = $(NVT_VOS_DIR)/drivers
# INCs
HDAL_INC_PATH = $(NVT_HDAL_DIR)/include
HDAL_LIB_PATH = $(NVT_HDAL_DIR)/output
VOS_LIB_PATH  = $(NVT_VOS_DIR)/output
AI_LIB        = $(NVT_HDAL_DIR)/vendor/ai2/source
MEDIA_LIB     = $(NVT_HDAL_DIR)/vendor/media/source

uclibc=$(shell echo $(CROSS_COMPILE)|grep uclib)
ifeq ($(uclibc),)
	PREBUILD_LIB=$(NVT_HDAL_DIR)/vendor/ai/source/prebuilt/lib/glibc
    PREBUILD_LIB_1=$(NVT_HDAL_DIR)/vendor/third_party/fdcnn_ai2/prebuilt/glibc
else
	PREBUILD_LIB=$(NVT_HDAL_DIR)/vendor/ai/source/prebuilt/lib/uclibc
    PREBUILD_LIB_1=$(NVT_HDAL_DIR)/vendor/third_party/fdcnn_ai2/prebuilt/uclibc
endif
# INC FLAGs
EXTRA_INCLUDE += -I$(HDAL_INC_PATH) -I$(NVT_HDAL_DIR)/source
EXTRA_INCLUDE += -I$(NVT_HDAL_DIR)/drivers/k_flow/include
EXTRA_INCLUDE += -I$(NVT_HDAL_DIR)/drivers/k_driver/include
EXTRA_INCLUDE += -I$(NVT_HDAL_DIR)/drivers/k_driver/source/include
EXTRA_INCLUDE += -I$(NVT_HDAL_DIR)/vendor/ai2/include
EXTRA_INCLUDE += -I$(NVT_HDAL_DIR)/vendor/ai2/source_pub/include
EXTRA_INCLUDE += -I$(NVT_HDAL_DIR)/vendor/ai2/source_pub/vendor_ai_cpu
EXTRA_INCLUDE += -I$(NVT_HDAL_DIR)/vendor/media/include
EXTRA_INCLUDE += -I$(NVT_HDAL_DIR)/vendor/ai/drivers/k_driver/include
EXTRA_INCLUDE += -I$(NVT_HDAL_DIR)/vendor/ai2/drivers/k_flow/include
EXTRA_INCLUDE += -I$(NVT_HDAL_DIR)/vendor/ai2/drivers/k_flow/source/kflow_ai_net
EXTRA_INCLUDE += -I$(NVT_HDAL_DIR)/vendor/third_party/fdcnn_ai2/include
EXTRA_INCLUDE += -I$(NVT_VOS_DIR)/drivers/include
EXTRA_INCLUDE += -I$(NVT_HDAL_DIR)/vendor/isp/include
.PHONY: all clean
###############################################################################
# Linux Makefile                                                              #
###############################################################################
ifeq ($(NVT_PRJCFG_CFG),Linux)
#--------- ENVIRONMENT SETTING --------------------
WARNING		= -Wall -Wundef -Wsign-compare -Wno-missing-braces -Wstrict-prototypes -Werror
COMPILE_OPTS	=  -I. -O2 -fPIC -ffunction-sections -fdata-sections -D__LINUX
C_CFLAGS	 = $(PLATFORM_CFLAGS) $(COMPILE_OPTS) $(WARNING) $(EXTRA_INCLUDE)
LD_FLAGS   	+= -L$(HDAL_LIB_PATH) -lpthread -lhdal
LD_FLAGS 	+= -L$(AI_LIB) -lvendor_ai2
LD_FLAGS 	+= -L$(AI_LIB)_pub -lvendor_ai2_pub
LD_FLAGS 	+= -L$(MEDIA_LIB) -lvendor_media
LD_FLAGS    += -L$(VOS_LIB_PATH) -lvos  
LD_FLAGS	+= -L$(PREBUILD_LIB) -lprebuilt_ai
LD_FLAGS	+= -L$(PREBUILD_LIB_1) -lprebuilt_fdcnn_ai2
LD_FLAGS	+= -L$(NVT_HDAL_DIR)/vendor/output -lvendor_isp 
#--------- END OF ENVIRONMENT SETTING -------------
LIB_NAME = $(MODULE_NAME)
SRC = alg_fdcnn_ai2_sample.c

OBJ = $(SRC:.c=.o)

ifeq ("$(wildcard *.c */*.c)","")
all:
	@echo "nothing to be done for '$(OUTPUT_NAME)'"
clean:
	@echo "nothing to be done for '$(OUTPUT_NAME)'"
else
all: $(LIB_NAME)

$(LIB_NAME): $(OBJ)
	@echo Creating $@...
	@$(CC) -o $@ $(OBJ) $(LD_FLAGS)
	@$(NM) -n $@ > $@.sym
	@$(STRIP) $@
	@$(OBJCOPY) -R .comment -R .note.ABI-tag -R .gnu.version $@

%.o: %.c
	@echo Compiling $<
	@$(CC) $(C_CFLAGS) -c $< -o $@

clean:
	@rm -f $(LIB_NAME) $(OBJ) $(LIB_NAME).sym *.o *.a *.so*
endif

install:
	@cp -avf $(LIB_NAME) $(ROOTFS_DIR)/rootfs/usr/bin

###############################################################################
# rtos Makefile                                                               #
###############################################################################
else ifeq ($(NVT_PRJCFG_CFG),rtos)
#--------- ENVIRONMENT SETTING --------------------
C_CFLAGS = $(PLATFORM_CFLAGS) $(EXTRA_INCLUDE)
#--------- END OF ENVIRONMENT SETTING -------------
LIB_NAME = lib$(MODULE_NAME).a
SRC = alg_fdcnn_ai2_sample.c

OBJ = $(SRC:.c=.o)

all: 
	@echo "nothing to be done for '$(LIB_NAME)'"
clean:
	@echo "nothing to be done for '$(LIB_NAME)'"

install:
	@echo "nothing to be done for '$(LIB_NAME)'"
endif