MODULE_NAME = libvos
# DIRs
OUTPUT_DIR = ../output
VOS_DRIVER_DIR = $(NVT_VOS_DIR)/drivers
VOS_LIBRARY_DIR = $(NVT_VOS_DIR)/source
# INCs
VOS_DRV_SRC_INC_PATH = $(VOS_DRIVER_DIR)/source/include
VOS_LIB_INC_PATH = $(NVT_VOS_DIR)/include
# INC FLAGs
EXTRA_INCLUDE += -I$(VOS_LIB_INC_PATH) -I$(VOS_DRV_SRC_INC_PATH)

.PHONY: all clean
###############################################################################
# Linux Makefile                                                              #
###############################################################################
ifeq ($(NVT_PRJCFG_CFG),Linux)
#--------- ENVIRONMENT SETTING --------------------
WARNING	= -Wall -Wundef -Wsign-compare -Wno-missing-braces -Wstrict-prototypes -Werror
COMPILE_OPTS	= -I. -O2 -fPIC -ffunction-sections -fdata-sections
C_PREDEFINED    = -D_NVT_CONSOLE_
C_CFLAGS	= $(PLATFORM_CFLAGS) $(COMPILE_OPTS) $(WARNING) $(EXTRA_INCLUDE) $(C_PREDEFINED)
LD_FLAGS	= -fPIC -shared -lpthread -lrt
#--------- END OF ENVIRONMENT SETTING -------------
SRC = \
	dummy/dummy.c \
	vos_user_cpu.c \
	vos_user_debug.c \
	vos_user_file.c \
	vos_user_flag.c \
	vos_user_mailbox.c \
	vos_user_perf.c \
	vos_user_semaphore.c \
	vos_user_task.c \
	vos_user_util.c \
	sscanf.c \
	cmdsys.c \
	examsys.c \
	verinfo.c \

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
C_CFLAGS = $(PLATFORM_CFLAGS) $(EXTRA_INCLUDE)
#--------- END OF ENVIRONMENT SETTING -------------
LIB_NAME = $(MODULE_NAME).a
SRC = \
 dummy/dummy.c

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
