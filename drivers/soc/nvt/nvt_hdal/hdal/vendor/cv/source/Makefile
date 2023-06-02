MODULE_NAME = libvendor_cv
# DIRs
CV_DRIVER_DIR = $(NVT_HDAL_DIR)/vendor/cv/drivers
CV_LIB_DIR = $(NVT_HDAL_DIR)/vendor/cv
KFLOW_DIR = $(NVT_HDAL_DIR)/drivers/k_flow
OUTPUT_DIR = $(NVT_HDAL_DIR)/vendor/output
VOS_DRIVER_DIR = $(NVT_VOS_DIR)/drivers

# INCs
HDAL_INC_PATH = $(NVT_HDAL_DIR)/include
CV_DRIVER_INC_PATH = $(CV_DRIVER_DIR)/include
CV_LIB_INC_PATH = $(CV_LIB_DIR)/include
KFLOW_INC_PATH = $(KFLOW_DIR)/include
VOS_INC_PATH = $(VOS_DRIVER_DIR)/include

# INC FLAGs
EXTRA_INCLUDE += -I$(HDAL_INC_PATH) -I$(CV_DRIVER_INC_PATH) -I$(CV_LIB_INC_PATH) -I$(KFLOW_INC_PATH) -I$(VOS_INC_PATH)
EXTRA_INCLUDE += -I$(CV_DRIVER_INC_PATH)/kdrv_md
EXTRA_INCLUDE += -I$(CV_DRIVER_INC_PATH)/kdrv_dis
EXTRA_INCLUDE += -I$(CV_LIB_INC_PATH)/vendor_dis
EXTRA_INCLUDE += -I$(CV_LIB_INC_PATH)/vendor_ive
EXTRA_INCLUDE += -I$(CV_DRIVER_INC_PATH)/kdrv_ive

.PHONY: all clean
###############################################################################
# Linux Makefile                                                              #
###############################################################################
ifeq ($(NVT_PRJCFG_CFG),Linux)
#--------- ENVIRONMENT SETTING --------------------
WARNING	= -Wall -Wundef -Wsign-compare -Wno-missing-braces -Wstrict-prototypes -Werror
COMPILE_OPTS	= -I. -O2 -fPIC -ffunction-sections -fdata-sections -D__LINUX_USER__ -D__LINUX
C_CFLAGS	= $(NVT_GCOV) $(PLATFORM_CFLAGS) $(COMPILE_OPTS) $(WARNING) $(EXTRA_INCLUDE)
LD_FLAGS	= $(NVT_GCOV) -fPIC -shared
#--------- END OF ENVIRONMENT SETTING -------------
SRC = \
 dummy/dummy.c \
 vendor_md/vendor_md.c \
 vendor_ive/vendor_ive.c \
 vendor_dis/vendor_dis.c
OBJ = $(SRC:.c=.o)

uclibc=$(shell echo $(CROSS_COMPILE)|grep uclib)
ifeq ($(uclibc),)
    ENV_LIBC=glibc
else
    ENV_LIBC=uclibc
endif

ifeq ("$(wildcard *.c */*.c)","")
all:
	@echo "nothing to be done for '$(OUTPUT_NAME)'"
clean:
	@echo "nothing to be done for '$(OUTPUT_NAME)'"
codesize:
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
codesize:
	@echo "nothing to be done for '$(OUTPUT_NAME)'"
endif

install:
	@mkdir -p $(OUTPUT_DIR)
	@cp -avf *.so* *.a $(OUTPUT_DIR)
	@cp -avf prebuilt/$(ENV_LIBC)/*.so  $(OUTPUT_DIR)
	@cp -avf prebuilt/$(ENV_LIBC)/*.a $(OUTPUT_DIR)
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
CV_DRIVER_DIR = $(NVT_HDAL_DIR)/vendor/cv/drivers
# INCs
CV_DRIVER_INC_PATH = $(CV_DRIVER_DIR)/include
# INC FLAGs
EXTRA_INCLUDE += \
	-I$(RTOS_LIB_DIR) \
	-I$(CV_DRIVER_INC_PATH) \
	-I$(RTOS_KERNEL_DIR)/portable/GCC/ARM_CA9  \
	-I$(RTOS_KERNEL_DIR)/include \
	-I$(RTOS_KERNEL_DIR)/include/private \
	-I$(RTOS_CURR_DEMO_DIR)/include \
	-I$(RTOS_POSIX_DIR)/include \
	-I$(RTOS_POSIX_DIR)/include/portable/novatek
#--------- ENVIRONMENT SETTING --------------------
C_CFLAGS = $(PLATFORM_CFLAGS) $(EXTRA_INCLUDE)
#--------- END OF ENVIRONMENT SETTING -------------
LIB_NAME = $(MODULE_NAME).a
SRC = \
 dummy/dummy.c \
 vendor_dis/vendor_dis.c \
 vendor_md/vendor_md.c \
 vendor_ive/vendor_ive.c \

OBJ = $(SRC:.c=.o)

ENV_LIBC=rtos

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
	@cp -avf prebuilt/$(ENV_LIBC)/*.a $(OUTPUT_DIR)
endif
