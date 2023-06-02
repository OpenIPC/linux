CROSS_COMPILE ?=arm-linux-gnueabihf-
CC = $(CROSS_COMPILE)gcc
STRIP = $(CROSS_COMPILE)strip
BUILD_DIR := .build
LIB_NAME=cam_os_wrapper
BRANCH_ID := $(shell git rev-parse --abbrev-ref HEAD 2> /dev/null | sed -e 's/\//_/g')

#LINUX_KERNEL_PATH ?= ../../../linux-3.18
LINUX_KERNEL_PATH ?= ../../kernel/API
KL_API_INCLUDE ?= $(LINUX_KERNEL_PATH)/drivers/mstar/include

ccflags-$(CONFIG_MS_JPE) := -Wall -Werror -g -O2 -ffunction-sections -funwind-tables -fstack-protector
ccflags-$(CONFIG_MS_JPE) += -fPIC -DPIC -DOMXILCOMPONENTSPATH=\"/system/lib\" -DCONFIG_DEBUG_LEVEL=32

ccflags-$(CONFIG_MS_JPE) += -DTEST_THREAD_ID_DETECTION
ccflags-$(CONFIG_MS_JPE) += -DCAM_OS_LINUX_USER
#CFLAGS += -D__USE_GNU
LDFLAGS += -O2 -Bdirect -Wl,--hash-style=gnu
LIBS := -ldl -lpthread

OMX_CORE_LIB_C_SRCS := \
	./src/cam_os_wrapper.c \

CINCLUDES := \
    -I$(KL_API_INCLUDE) \
    -I./pub \
    -I./inc \

LIB_HEADERS := \
	./pub/cam_os_wrapper.h \
	./pub/cam_os_util_list.h \

OMX_CORE_LIB_C_OBJS := $(patsubst %.c, %.c.so.o,  $(OMX_CORE_LIB_C_SRCS))

.PHONY: clean prepare lib$(LIB_NAME)

all: prepare lib$(LIB_NAME)

prepare:
	@echo
	@echo ">>>>========================================================"
	@echo
	@echo "  LIB_NAME = ${LIB_NAME}"
	@echo "  PWD = $(shell pwd)"
	@echo "  CC = $(CC)"
	@echo "  BRANCH_ID = $(BRANCH_ID)"
	@echo
	@mkdir -p $(BUILD_DIR)
	@echo "  Copying headers to '$(BUILD_DIR)/include'..."
	@echo
	@mkdir -p $(BUILD_DIR)/include
	@cp -f $(LIB_HEADERS) $(BUILD_DIR)/include

install:
	@echo "  INSTALL $(LIB_NAME) to $(INSTALLDIR)"
	@rm    -Rf $(INSTALLDIR)
	@mkdir -p $(INSTALLDIR)
	@cp -Rf  $(BUILD_DIR)/lib $(INSTALLDIR)
	@cp -Rf  $(BUILD_DIR)/include $(INSTALLDIR)

clean:
	@rm -Rf $(OMX_CORE_LIB_C_OBJS)
	@rm -Rf $(BUILD_DIR)

lib$(LIB_NAME):	prepare $(OMX_CORE_LIB_C_OBJS)
	@mkdir -p $(BUILD_DIR)/lib
	@echo "  LD      $(BUILD_DIR)/lib/$@.so"
	@$(CC) -shared $(LDFLAGS) -o $(BUILD_DIR)/lib/$@.so $(OMX_CORE_LIB_C_OBJS) ${LIBS}
	@echo
	@echo "<<<<========================================================"
	@echo

%.c.o : %.c
	@mkdir -p $(dir $@)
	@echo "  CC      $<"
#	@echo $(CFLAGS) $(CINCLUDES)  -c $< -o $@
	@$(CC) $(CFLAGS) $(CINCLUDES)  -c $< -o $@

%.c.test.o : %.c
	@mkdir -p $(dir $@)
	@echo "  CC      $<"
	@$(CC) $(CFLAGS) $(TEST_CINCLUDES)  -c $< -o $@

%.c.so.o : %.c
	@mkdir -p $(dir $@)
	@echo "  CC      $<"
#	@echo $(CFLAGS) $(CINCLUDES)  -c $< -o $@
	@$(CC) $(CFLAGS) $(CINCLUDES) -c $< -o $@