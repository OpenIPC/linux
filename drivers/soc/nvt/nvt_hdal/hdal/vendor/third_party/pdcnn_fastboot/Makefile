CUR_PATH := $(shell pwd)
OUTPUT_DIR = $(NVT_HDAL_DIR)/vendor/output

uclibc=$(shell echo $(CROSS_COMPILE)|grep uclib)
ifeq ($(uclibc),)
    PREBUILD_LIB=$(NVT_HDAL_DIR)/vendor/third_party/pdcnn_fastboot/prebuilt/glibc
else
    PREBUILD_LIB=$(NVT_HDAL_DIR)/vendor/third_party/pdcnn_fastboot/prebuilt/uclibc
endif

###############################################################################
# Linux Makefile                                                              #
###############################################################################
ifeq ($(NVT_PRJCFG_CFG),Linux)
all: 
	@echo "nothing to be done for '$(CUR_PATH)'"

install:
	cp $(PREBUILD_LIB)/*.so $(PREBUILD_LIB)/*.a $(OUTPUT_DIR)

clean :
	@echo "nothing to be done for '$(CUR_PATH)'"

###############################################################################
# rtos Makefile                                                               #
###############################################################################
else ifeq ($(NVT_PRJCFG_CFG),rtos)
all: 
	@echo "nothing to be done for '$(LIB_NAME)'"

clean:
	@echo "nothing to be done for '$(LIB_NAME)'"

install:
	@mkdir -p $(OUTPUT_DIR)
	@cp -avf prebuilt/rtos/*.a $(OUTPUT_DIR)
endif