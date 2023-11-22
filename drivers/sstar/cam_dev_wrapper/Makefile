#
# Copyright (c) [2019~2020] SigmaStar Technology.
#
#
# This software is licensed under the terms of the GNU General Public
# License version 2, as published by the Free Software Foundation, and
# may be copied, distributed, and modified under those terms.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License version 2 for more details.
#

CROSS_COMPILE ?=arm-linux-gnueabihf-
CC = $(CROSS_COMPILE)gcc
STRIP = $(CROSS_COMPILE)strip
BUILD_DIR := .build
LIB_NAME=cam_dev_wrapper
BRANCH_ID := $(shell git rev-parse --abbrev-ref HEAD 2> /dev/null | sed -e 's/\//_/g')

#LINUX_KERNEL_PATH ?= ../../../linux-3.18
LINUX_KERNEL_PATH ?= ../../kernel/API
KL_API_INCLUDE ?= $(LINUX_KERNEL_PATH)/drivers/sstar/include

CFLAGS := -Wall -Werror -g -O2 -ffunction-sections -funwind-tables -fstack-protector
CFLAGS += -fPIC -DPIC -DCONFIG_DEBUG_LEVEL=32

CFLAGS += -DTEST_THREAD_ID_DETECTION
CFLAGS += -DCAM_OS_LINUX_USER
#CFLAGS += -D__USE_GNU
LDFLAGS += -O2 -Bdirect -Wl,--hash-style=gnu
LIBS := -ldl -lpthread


CAM_DEV_WRAPPER_LIB_C_SRCS := \
    ./src/cam_dev_wrapper.c \

CINCLUDES := \
    -I$(KL_API_INCLUDE) \
    -I./pub \
    -I./inc \
    -I../cam_os_wrapper/pub \

LIB_HEADERS := \
	./pub/cam_dev_wrapper.h \
	./pub/cam_dev_scl.h \

CAM_DEV_WRAPPER_LIB_C_OBJS := $(patsubst %.c, %.c.so.o,  $(CAM_DEV_WRAPPER_LIB_C_SRCS))


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
	@rm -Rf $(CAM_DEV_WRAPPER_LIB_C_OBJS)
	@rm -Rf $(BUILD_DIR)



lib$(LIB_NAME):	prepare $(CAM_DEV_WRAPPER_LIB_C_OBJS)
	@mkdir -p $(BUILD_DIR)/lib
	@echo "  LD      $(BUILD_DIR)/lib/$@.so"
	@$(CC) -shared $(LDFLAGS) -o $(BUILD_DIR)/lib/$@.so $(CAM_DEV_WRAPPER_LIB_C_OBJS) ${LIBS}
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
