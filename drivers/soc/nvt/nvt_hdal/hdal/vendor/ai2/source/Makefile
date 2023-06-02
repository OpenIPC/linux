CUR_PATH := $(shell pwd)
NVT_KDRV_PATH=$(NVT_HDAL_DIR)/drivers/k_driver
NVT_KFLOW_PATH=$(NVT_HDAL_DIR)/drivers/k_flow
NVT_AI_PATH=$(NVT_HDAL_DIR)/vendor/ai
NVT_AI2_PATH=$(NVT_HDAL_DIR)/vendor/ai2
NVT_AI_KDRV_PATH=$(NVT_AI_PATH)/drivers/k_driver
NVT_AI_KFLOW_PATH=$(NVT_AI2_PATH)/drivers/k_flow
NVT_AI_LIB_PATH=$(NVT_AI2_PATH)/source
OUTPUT_DIR = $(NVT_HDAL_DIR)/vendor/output

INCLUDE_PATH += -I$(NVT_VOS_DIR)/drivers/include
INCLUDE_PATH += -I$(NVT_HDAL_DIR)/include
INCLUDE_PATH += -I$(NVT_KDRV_PATH)/include -I$(NVT_KDRV_PATH)/source/include/ -I$(NVT_KFLOW_PATH)/include/
INCLUDE_PATH += -I$(NVT_HDAL_DIR)/vendor/include -I$(NVT_AI_LIB_PATH)/include -I$(NVT_AI2_PATH)/include -I$(NVT_AI_KDRV_PATH)/include -I$(NVT_AI_KFLOW_PATH)/include
INCLUDE_PATH += -I$(NVT_HDAL_DIR)/vendor/ai2/source/prebuilt/include
INCLUDE_PATH += -I$(NVT_HDAL_DIR)/vendor/cv/include -I$(NVT_HDAL_DIR)/vendor/isp/include
INCLUDE_PATH += -I$(NVT_HDAL_DIR)/vendor/dsp/include
INCLUDE_PATH += -I$(NVT_HDAL_DIR)/vendor/media/include

#COPS = $(INCLUDE_PATH) -D ARM_ASM -O3 -Wall -Werror -finline-functions -msoft-float -fPIC -Wno-aggressive-loop-optimizations
COPS = $(INCLUDE_PATH) -D ARM_ASM -O3 -Wall -Werror -Werror=sign-compare -finline-functions -fPIC -Wno-aggressive-loop-optimizations -D__LINUX_USER__ -mfpu=neon-fp16

uclibc=$(shell echo $(CROSS_COMPILE)|grep uclib)
ifeq ($(uclibc),)
    PREBUILD_LIB=$(NVT_HDAL_DIR)/vendor/ai/source/prebuilt/lib/glibc
else
    PREBUILD_LIB=$(NVT_HDAL_DIR)/vendor/ai/source/prebuilt/lib/uclibc
endif

CC=$(CROSS_COMPILE)gcc
AS=$(CROSS_COMPILE)as
AR=$(CROSS_COMPILE)ar

CSRC += \
	vendor_ai_comm.c \
	vendor_ai_comm_flow.c \
	vendor_ai_net/vendor_ai_op.c \
	vendor_ai_net/vendor_ai_net_gen.c \
	vendor_ai_net/vendor_ai_net_flow.c \
	vendor_ai_net/vendor_ai_net.c \
	vendor_ai_net/vendor_ai_net_mem.c \
	vendor_ai_net/vendor_ai_net_group.c \
	vendor_ai_net/vendor_ai_net_debug.c \
	vendor_ai_net/vendor_ai_net_cmd.c \
	vendor_ai_net/debug_util/graph_debug_core.c \
	vendor_ai_net/debug_util/graph_debug_buffer.c \
	vendor_ai_net/debug_util/graph_debug_schedule.c \
	vendor_ai_cpu/vendor_ai_cpu_thread.c \
	vendor_ai_cpu/vendor_ai_cpu_post_accuracy.c \
	vendor_ai_cpu/vendor_ai_cpu_post_softmax.c \
	vendor_ai_dsp/vendor_ai_dsp_thread.c \
	vendor_ai_dla/vendor_ai_dla.c

	
HDAL_STATIC_LIB = libvendor_ai2.a
HDAL_DYNAMIC_LIB = libvendor_ai2.so

COBJS = ${CSRC:%.c=%.o} ${CPPSRC:%.cpp=%.o}

TARGETS := $(HDAL_STATIC_LIB) $(HDAL_DYNAMIC_LIB)

.PHONY: $(TARGETS)
###############################################################################
# Linux Makefile                                                              #
###############################################################################
ifeq ($(NVT_PRJCFG_CFG),Linux)
ifeq ("$(wildcard *.c */*.c)","")
all:
	@echo "nothing to be done for '$(OUTPUT_NAME)'"
clean:
	@echo "nothing to be done for '$(OUTPUT_NAME)'"
codesize:
	@echo "nothing to be done for '$(OUTPUT_NAME)'"		
else
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

clean :
	/bin/rm -f *.o
	/bin/rm -f */*.o
	/bin/rm -f */*/*.o
	/bin/rm -f $(HDAL_STATIC_LIB) $(HDAL_DYNAMIC_LIB)
endif

install:
#	cp *.so *.a $(ROOTFS_DIR)/rootfs/lib
	cp *.so *.a $(OUTPUT_DIR)
#	cp $(PREBUILD_LIB)/*.so $(PREBUILD_LIB)/*.a $(ROOTFS_DIR)/rootfs/lib
	cp $(PREBUILD_LIB)/*.so $(PREBUILD_LIB)/*.a $(OUTPUT_DIR)

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
#--------- END OF ENVIRONMENT SETTING -------------
MODULE_NAME = libvendor_ai2
LIB_NAME = $(MODULE_NAME).a
PREBUILD_LIB=$(NVT_HDAL_DIR)/vendor/ai/source/prebuilt/lib

SRC = \
	vendor_ai_comm.c \
	vendor_ai_comm_flow.c \
	vendor_ai_net/vendor_ai_op.c \
	vendor_ai_net/vendor_ai_net_gen.c \
	vendor_ai_net/vendor_ai_net_flow.c \
	vendor_ai_net/vendor_ai_net.c \
	vendor_ai_net/vendor_ai_net_mem.c \
	vendor_ai_net/vendor_ai_net_group.c \
	vendor_ai_net/vendor_ai_net_debug.c \
	vendor_ai_net/vendor_ai_net_cmd.c \
	vendor_ai_net/debug_util/graph_debug_core.c \
	vendor_ai_net/debug_util/graph_debug_buffer.c \
	vendor_ai_net/debug_util/graph_debug_schedule.c \
	vendor_ai_cpu/vendor_ai_cpu_thread.c \
	vendor_ai_cpu/vendor_ai_cpu_post_accuracy.c \
	vendor_ai_cpu/vendor_ai_cpu_post_softmax.c \
	vendor_ai_dsp/vendor_ai_dsp_thread.c \
	vendor_ai_dla/vendor_ai_dla.c

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
codesize:
	@echo $(MODULE_NAME) no obj

install:
	@mkdir -p $(OUTPUT_DIR)
	@cp -avf $(LIB_NAME) $(OUTPUT_DIR)
	@cp -avf $(PREBUILD_LIB)/rtos/*.a $(OUTPUT_DIR)
	
endif
