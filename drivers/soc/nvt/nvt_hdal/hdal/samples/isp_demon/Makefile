MODULE_NAME = isp_demon
# DIRs
VOS_DRIVER_DIR = $(NVT_VOS_DIR)/drivers
HDAL_SAMPLE_DIR = $(NVT_HDAL_DIR)/samples
RTOS_OUTPUT_DIR = $(HDAL_SAMPLE_DIR)/output
# INCs
VOS_INC_PATH = $(VOS_DRIVER_DIR)/include
HDAL_INC_PATH = $(NVT_HDAL_DIR)/include
HDAL_LIB_PATH = $(NVT_HDAL_DIR)/output
# INC FLAGs
EXTRA_INCLUDE += -I$(HDAL_INC_PATH) -I$(VOS_INC_PATH)
.PHONY: all clean
###############################################################################
# Linux Makefile                                                              #
###############################################################################
ifeq ($(NVT_PRJCFG_CFG),Linux)
#--------- ENVIRONMENT SETTING --------------------
WARNING		= -Wall -Wundef -Wsign-compare -Wno-missing-braces -Wstrict-prototypes -Werror
COMPILE_OPTS	=  -I. -O2 -fPIC -ffunction-sections -fdata-sections -D__LINUX
C_CFLAGS	= $(PLATFORM_CFLAGS) $(COMPILE_OPTS) $(WARNING) $(EXTRA_INCLUDE)
LD_FLAGS	= -L$(HDAL_LIB_PATH) -lhdal -lpthread
#--------- END OF ENVIRONMENT SETTING -------------

#--------- Compiling -------------------
uclibc=$(shell echo $(CROSS_COMPILE)|grep uclib)
ifeq ($(uclibc),)
    BIN=isp_demon_glibc
else
    BIN=isp_demon_uclibc
endif

.PHONY: install

clean:

install:
	@cp -avf $(BIN) $(ROOTFS_DIR)/rootfs/usr/bin/isp_demon

###############################################################################
# rtos Makefile                                                               #
###############################################################################
else ifeq ($(NVT_PRJCFG_CFG),rtos)

all:
	@echo "nothing to be done for '$(MODULE_NAME)'"



clean:
	@echo "nothing to be done for '$(MODULE_NAME)'"

install:
	@mkdir -p $(RTOS_OUTPUT_DIR)
	@cp -avf ./rtos/lib$(MODULE_NAME).a $(RTOS_OUTPUT_DIR)
	
endif