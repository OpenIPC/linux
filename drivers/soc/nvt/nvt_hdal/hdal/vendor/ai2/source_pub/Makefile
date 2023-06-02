CUR_PATH := $(shell pwd)
NVT_AI_PATH=$(NVT_HDAL_DIR)/vendor/ai
NVT_AI2_PATH=$(NVT_HDAL_DIR)/vendor/ai2
NVT_KDRV_PATH=$(NVT_HDAL_DIR)/drivers/k_driver
NVT_KFLOW_PATH=$(NVT_HDAL_DIR)/drivers/k_flow
NVT_AI_KDRV_PATH=$(NVT_AI_PATH)/drivers/k_driver
NVT_AI_KFLOW_PATH=$(NVT_AI2_PATH)/drivers/k_flow
OUTPUT_DIR = $(NVT_HDAL_DIR)/vendor/output

INCLUDE_PATH += -I$(NVT_VOS_DIR)/drivers/include
INCLUDE_PATH += -I$(NVT_HDAL_DIR)/include -I$(NVT_KDRV_PATH)/include -I$(NVT_KDRV_PATH)/source/include/ -I$(NVT_KFLOW_PATH)/include/
INCLUDE_PATH += -I$(NVT_HDAL_DIR)/vendor/include -I$(NVT_AI2_PATH)/include -I$(NVT_AI2_PATH)/source_pub/include
INCLUDE_PATH += -I$(NVT_AI2_PATH)/source/prebuilt/include -I$(NVT_AI_KDRV_PATH)/include -I$(NVT_AI_KFLOW_PATH)/include
INCLUDE_PATH += -I$(NVT_HDAL_DIR)/vendor/cv/include -I$(NVT_HDAL_DIR)/vendor/isp/include
INCLUDE_PATH += -I$(NVT_HDAL_DIR)/vendor/dsp/include
INCLUDE_PATH += -I$(NVT_HDAL_DIR)/vendor/ai2/source/include
INCLUDE_PATH += -I$(NVT_HDAL_DIR)/vendor/ai2/source/vendor_ai_net

#COPS = $(INCLUDE_PATH) -D ARM_ASM -O3 -Wall -Werror -finline-functions -msoft-float -fPIC -Wno-aggressive-loop-optimizations
COPS = $(INCLUDE_PATH) -D ARM_ASM -O3 -Wall -Werror -finline-functions -fPIC -Wno-aggressive-loop-optimizations -mfpu=neon-fp16

CC=$(CROSS_COMPILE)gcc
AS=$(CROSS_COMPILE)as
AR=$(CROSS_COMPILE)ar

CSRC += \
	vendor_ai_util.c \
	vendor_ai_cpu/vendor_ai_cpu.c \
	vendor_ai_cpu/vendor_ai_cpu_custnn.c \
	vendor_ai_cpu/vendor_ai_cpu_custnn_eltwise.c \
	vendor_ai_cpu/vendor_ai_cpu_custnn_concat.c \
	vendor_ai_cpu/vendor_ai_cpu_custnn_slice.c \
	vendor_ai_cpu/vendor_ai_cpu_detout.c \
	vendor_ai_cpu/vendor_ai_cpu_nvtnn.c \
	vendor_ai_cpu/vendor_ai_cpu_postproc.c \
	vendor_ai_cpu/vendor_ai_cpu_prelu.c \
	vendor_ai_cpu/vendor_ai_cpu_priorbox.c \
	vendor_ai_cpu/vendor_ai_cpu_proposal.c \
	vendor_ai_cpu/vendor_ai_cpu_softmax.c \
	vendor_ai_cpu/vendor_ai_cpu_util.c \
	vendor_ai_dsp/vendor_ai_dsp.c \
	vendor_ai_dsp/vendor_ai_dsp_custnn.c \
	vendor_ai_dsp/vendor_ai_dsp_custnn_pool.c \
	vendor_ai_dsp/vendor_ai_dsp_nvtnn.c

CPPSRC += \
	vendor_ai_cpu/vendor_ai_cpu_custnn_pool.cpp


HDAL_STATIC_LIB = libvendor_ai2_pub.a
HDAL_DYNAMIC_LIB = libvendor_ai2_pub.so

COBJS = ${CSRC:%.c=%.o} ${CPPSRC:%.cpp=%.o}

TARGETS := $(HDAL_STATIC_LIB) $(HDAL_DYNAMIC_LIB)

.PHONY: $(TARGETS)
###############################################################################
# Linux Makefile                                                              #
###############################################################################
ifeq ($(NVT_PRJCFG_CFG),Linux)
all: $(TARGETS)

$(HDAL_STATIC_LIB) : $(COBJS) $(SOBJS) 
	$(AR) -r $@ $(COBJS)
#	mkdir -p ./lib
#	cp $@ ./lib/

$(HDAL_DYNAMIC_LIB) : $(COBJS) $(SOBJS) 
	$(CC) -shared -s -o $@ $(COBJS)
#	mkdir -p ../lib
#	cp $@ ./lib/

%.o: %.c
	$(CC) -c $(COPS) $(CFLAGS) -I. -o $@ $<
	
%.o: %.cpp
	$(CC) -c $(COPS) $(CFLAGS) -I. -o $@ $<

install:
	cp *.so *.a $(OUTPUT_DIR)

clean :
	/bin/rm -f *.o
	/bin/rm -f */*.o
	/bin/rm -f $(HDAL_STATIC_LIB) $(HDAL_DYNAMIC_LIB)

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

C_CFLAGS = $(PLATFORM_CFLAGS) $(INCLUDE_PATH) $(EXTRA_INCLUDE) -Wno-format
C_CXXFLAGS = $(PLATFORM_CXXFLAGS) $(INCLUDE_PATH) $(EXTRA_INCLUDE) -Wno-format
#--------- END OF ENVIRONMENT SETTING -------------
MODULE_NAME = libvendor_ai2_pub
LIB_NAME = $(MODULE_NAME).a
PREBUILD_LIB=$(NVT_HDAL_DIR)/vendor/ai/source/prebuilt/lib

SRC = \
	vendor_ai_util.c \
	vendor_ai_cpu/vendor_ai_cpu.c \
	vendor_ai_cpu/vendor_ai_cpu_custnn.c \
	vendor_ai_cpu/vendor_ai_cpu_custnn_eltwise.c \
	vendor_ai_cpu/vendor_ai_cpu_custnn_concat.c \
	vendor_ai_cpu/vendor_ai_cpu_custnn_slice.c \
	vendor_ai_cpu/vendor_ai_cpu_detout.c \
	vendor_ai_cpu/vendor_ai_cpu_nvtnn.c \
	vendor_ai_cpu/vendor_ai_cpu_postproc.c \
	vendor_ai_cpu/vendor_ai_cpu_prelu.c \
	vendor_ai_cpu/vendor_ai_cpu_priorbox.c \
	vendor_ai_cpu/vendor_ai_cpu_proposal.c \
	vendor_ai_cpu/vendor_ai_cpu_softmax.c \
	vendor_ai_cpu/vendor_ai_cpu_util.c \
	vendor_ai_dsp/vendor_ai_dsp.c \
	vendor_ai_dsp/vendor_ai_dsp_custnn.c \
	vendor_ai_dsp/vendor_ai_dsp_custnn_pool.c \
	vendor_ai_dsp/vendor_ai_dsp_nvtnn.c

PSRC += \
	vendor_ai_cpu/vendor_ai_cpu_custnn_pool.cpp
	
OBJ = $(SRC:.c=.o) $(PSRC:.cpp=.o)


all: $(LIB_NAME)

$(LIB_NAME): $(OBJ)
	@echo Creating $@...
	@$(AR) rcsD $@ $(OBJ)
	@$(BUILD_DIR)/nvt-tools/nvt-ld-op --arc-sha1 $@

%.o: %.c
	@echo Compiling $<
	@$(CC) $(C_CFLAGS) -c $< -o $@
	
%.o: %.cpp
	@echo Compiling $<
	@$(CXX) $(C_CXXFLAGS) -c $(COPS) $< -o $@
	
clean:
	@rm -f $(LIB_NAME) $(OBJ) $(LIB_NAME).sym *.o *.a *.so*
	@rm -rf $(OUTPUT_DIR)

install:
	@mkdir -p $(OUTPUT_DIR)
	@cp -avf $(LIB_NAME) $(OUTPUT_DIR)
	@cp -avf $(PREBUILD_LIB)/rtos/*.a $(OUTPUT_DIR)
	
endif
