MODULE_NAME = ai2_pdcnn_fdcnn_cddcnn_sample_stream_ipc

# DIRs
VOS_DRIVER_DIR = $(NVT_VOS_DIR)/drivers
HDAL_SAMPLE_DIR = $(NVT_HDAL_DIR)/samples
RTOS_OUTPUT_DIR = $(HDAL_SAMPLE_DIR)/output
HDAL_VENDOR_LIB_PATH = $(NVT_HDAL_DIR)/vendor/output

# INCs
VOS_INC_PATH = $(VOS_DRIVER_DIR)/include
HDAL_INC_PATH = $(NVT_HDAL_DIR)/include
HDAL_LIB_PATH = $(NVT_HDAL_DIR)/output
VOS_LIB_PATH  = $(NVT_VOS_DIR)/output
AI_LIB        = $(NVT_HDAL_DIR)/vendor/ai2/source

uclibc=$(shell echo $(CROSS_COMPILE)|grep uclib)
ifeq ($(uclibc),)
    PREBUILD_LIB=$(NVT_HDAL_DIR)/vendor/ai/source/prebuilt/lib/glibc
    PREBUILD_LIB_1=$(NVT_HDAL_DIR)/vendor/third_party/pdcnn_ai2/prebuilt/glibc
    PREBUILD_LIB_2=$(NVT_HDAL_DIR)/vendor/third_party/fdcnn_ai2/prebuilt/glibc
    PREBUILD_LIB_3=$(NVT_HDAL_DIR)/vendor/third_party/cddcnn_ai2/prebuilt/glibc
    PREBUILD_LIB_4=$(NVT_HDAL_DIR)/vendor/third_party/limit_fdet/prebuilt/glibc
else
    PREBUILD_LIB=$(NVT_HDAL_DIR)/vendor/ai/source/prebuilt/lib/uclibc
    PREBUILD_LIB_1=$(NVT_HDAL_DIR)/vendor/third_party/pdcnn_ai2/prebuilt/uclibc
    PREBUILD_LIB_2=$(NVT_HDAL_DIR)/vendor/third_party/fdcnn_ai2/prebuilt/uclibc
    PREBUILD_LIB_3=$(NVT_HDAL_DIR)/vendor/third_party/cddcnn_ai2/prebuilt/uclibc
    PREBUILD_LIB_4=$(NVT_HDAL_DIR)/vendor/third_party/limit_fdet/prebuilt/uclibc
endif

# INC FLAGs
EXTRA_INCLUDE += -I$(HDAL_INC_PATH) -I$(VOS_INC_PATH)
EXTRA_INCLUDE += -I$(HDAL_INC_PATH) -I$(NVT_HDAL_DIR)/source
EXTRA_INCLUDE += -I$(NVT_HDAL_DIR)/drivers/k_flow/include
EXTRA_INCLUDE += -I$(NVT_HDAL_DIR)/drivers/k_driver/include
EXTRA_INCLUDE += -I$(NVT_HDAL_DIR)/drivers/k_driver/source/include
EXTRA_INCLUDE += -I$(NVT_HDAL_DIR)/vendor/ai2/include
EXTRA_INCLUDE += -I$(NVT_HDAL_DIR)/vendor/ai2/source_pub/include
EXTRA_INCLUDE += -I$(NVT_HDAL_DIR)/vendor/ai2/source_pub/vendor_ai_cpu
EXTRA_INCLUDE += -I$(NVT_HDAL_DIR)/vendor/ai/drivers/k_driver/include
EXTRA_INCLUDE += -I$(NVT_HDAL_DIR)/vendor/ai2/drivers/k_flow/include
EXTRA_INCLUDE += -I$(NVT_HDAL_DIR)/vendor/ai2/drivers/k_flow/source/kflow_ai_net
EXTRA_INCLUDE += -I$(NVT_HDAL_DIR)/vendor/third_party/pdcnn_ai2/include
EXTRA_INCLUDE += -I$(NVT_HDAL_DIR)/vendor/third_party/fdcnn_ai2/include
EXTRA_INCLUDE += -I$(NVT_HDAL_DIR)/vendor/third_party/cddcnn_ai2/include
EXTRA_INCLUDE += -I$(NVT_HDAL_DIR)/vendor/third_party/limit_fdet/include
EXTRA_INCLUDE += -I$(NVT_VOS_DIR)/drivers/include -I$(NVT_HDAL_DIR)/vendor/media/include
EXTRA_INCLUDE += -I$(NVT_HDAL_DIR)/vendor/isp/include

.PHONY: all clean

###############################################################################
# Linux Makefile                                                              #
###############################################################################
ifeq ($(NVT_PRJCFG_CFG),Linux)
#--------- ENVIRONMENT SETTING --------------------
WARNING		= -Wall -Wundef -Wsign-compare -Wno-missing-braces -Wstrict-prototypes -Werror
COMPILE_OPTS	=  -I. -O2 -fPIC -ffunction-sections -fdata-sections -D__LINUX
C_CFLAGS	= $(PLATFORM_CFLAGS) $(COMPILE_OPTS) $(WARNING) $(EXTRA_INCLUDE)
LD_FLAGS	= -L$(HDAL_LIB_PATH) -L$(HDAL_VENDOR_LIB_PATH) -lhdal -lm -lpthread -lvendor_media
LD_FLAGS 	+= -L$(AI_LIB) -lvendor_ai2
LD_FLAGS 	+= -L$(AI_LIB)_pub -lvendor_ai2_pub
LD_FLAGS    += -L$(VOS_LIB_PATH) -lvos 
LD_FLAGS	+= -L$(PREBUILD_LIB) -lprebuilt_ai
LD_FLAGS	+= -L$(PREBUILD_LIB_1) -lprebuilt_pdcnn_ai2
LD_FLAGS	+= -L$(PREBUILD_LIB_2) -lprebuilt_fdcnn_ai2
LD_FLAGS	+= -L$(PREBUILD_LIB_3) -lprebuilt_cddcnn_ai2
LD_FLAGS	+= -L$(PREBUILD_LIB_4) -lprebuilt_limit_fdet
LD_FLAGS	+= -L$(NVT_HDAL_DIR)/vendor/output -lvendor_isp
#--------- END OF ENVIRONMENT SETTING -------------
LIB_NAME = $(MODULE_NAME)
SRC = ai2_pdcnn_fdcnn_cddcnn_sample_stream_ipc.c

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

C_CFLAGS = $(PLATFORM_CFLAGS) $(EXTRA_INCLUDE)
LIB_NAME = lib$(MODULE_NAME).a
SRC = alg_pdcnn_fdcnn_cddcnn_sample_stream_ipc.c

OBJ = $(SRC:.c=.o)

all: 
	@echo "nothing to be done for '$(LIB_NAME)'"
clean:
	@echo "nothing to be done for '$(LIB_NAME)'"

install:
	@echo "nothing to be done for '$(LIB_NAME)'"

###--------- ENVIRONMENT SETTING --------------------
### DIRs
##RTOS_KERNEL_DIR = $(KERNELDIR)/lib/FreeRTOS
##RTOS_LIB_DIR = $(KERNELDIR)/lib
##RTOS_CURR_DEMO_DIR = $(KERNELDIR)/demos/novatek/na51055
##RTOS_POSIX_DIR = $(KERNELDIR)/lib/FreeRTOS-Plus-POSIX
##RTOS_POSIX_SRC_DIR = $(RTOS_POSIX_DIR)/source

###INCs for C_CFLAGS

##EXTRA_INCLUDE += \
##	-I$(RTOS_LIB_DIR) \
##	-I$(RTOS_KERNEL_DIR)/portable/GCC/ARM_CA9  \
##	-I$(RTOS_KERNEL_DIR)/include \
##	-I$(RTOS_KERNEL_DIR)/include/private \
##	-I$(RTOS_CURR_DEMO_DIR)/include \
##	-I$(RTOS_POSIX_DIR)/include \
##	-I$(RTOS_POSIX_DIR)/include/portable/novatek \
##	-I$(LIBRARY_DIR)/include

##C_CFLAGS = $(PLATFORM_CFLAGS) $(EXTRA_INCLUDE) -Wno-format
###--------- END OF ENVIRONMENT SETTING -------------
##LIB_NAME = lib$(MODULE_NAME).a
##SRC = ai2_pdcnn_fdcnn_cddcnn_sample_stream_ipc.c

##OBJ = $(SRC:.c=.o)

##all: $(LIB_NAME)

##$(LIB_NAME): $(OBJ)
##	@echo Creating $@...
##	@$(AR) rcsD $@ $(OBJ)
##	@$(BUILD_DIR)/nvt-tools/nvt-ld-op --arc-sha1 $@

##%.o: %.c
##	@echo Compiling $<
##	@$(CC) $(C_CFLAGS) -c $< -o $@

##clean:
##	@rm -f $(LIB_NAME) $(OBJ) $(LIB_NAME).sym *.o *.a *.so*

##install:
##	@mkdir -p $(RTOS_OUTPUT_DIR)
##	@cp -avf $(LIB_NAME) $(RTOS_OUTPUT_DIR)
	
	
endif
