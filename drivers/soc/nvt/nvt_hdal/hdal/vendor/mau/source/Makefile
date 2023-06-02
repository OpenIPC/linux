MODULE_NAME = libvendor_mau
# DIRs
MAU_DRIVER_DIR = $(NVT_HDAL_DIR)/drivers/k_driver/source/comm/ddr_arb
MAU_LIB_DIR = $(NVT_HDAL_DIR)/vendor/mau
KFLOW_DIR = $(NVT_HDAL_DIR)/drivers/k_flow
OUTPUT_DIR = $(NVT_HDAL_DIR)/vendor/output
VOS_DRIVER_DIR = $(NVT_VOS_DIR)/drivers

# INCs
HDAL_INC_PATH = $(NVT_HDAL_DIR)/include
MAU_DRIVER_INC_PATH = $(MAU_DRIVER_DIR)/include
MAU_LIB_INC_PATH = $(MAU_LIB_DIR)/include
KFLOW_INC_PATH = $(KFLOW_DIR)/include
VOS_INC_PATH = $(VOS_DRIVER_DIR)/include

# INC FLAGs
EXTRA_INCLUDE += -I$(HDAL_INC_PATH) -I$(MAU_DRIVER_INC_PATH) -I$(MAU_LIB_INC_PATH) -I$(KFLOW_INC_PATH) -I$(VOS_INC_PATH)

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
 vendor_mau.c

uclibc=$(shell echo $(CROSS_COMPILE)|grep uclib)
ifeq ($(uclibc),)
    ENV_LIBC=glibc
else
    ENV_LIBC=uclibc
endif

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
ifneq ("$(wildcard prebuilt/$(ENV_LIBC)/*.a)","")
	@cp -avf prebuilt/$(ENV_LIBC)/*.* $(OUTPUT_DIR)
endif

###############################################################################
# rtos Makefile                                                               #
###############################################################################
else ifeq ($(NVT_PRJCFG_CFG),rtos)
#--------- ENVIRONMENT SETTING --------------------
C_CFLAGS = $(PLATFORM_CFLAGS) $(EXTRA_INCLUDE)
#--------- END OF ENVIRONMENT SETTING -------------
LIB_NAME = $(MODULE_NAME).a
SRC = \


ENV_LIBC=rtos

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
ifneq ("$(wildcard prebuilt/$(ENV_LIBC)/*.a)","")
	@cp -avf prebuilt/$(ENV_LIBC)/*.* $(OUTPUT_DIR)
endif
endif
