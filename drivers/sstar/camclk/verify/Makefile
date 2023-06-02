#
# Makefile for kernel test
#
PWD         ?= $(shell pwd)
DIR_PATH    := $(PWD)/../
KVERSION    := $(shell uname -r)

MODULE_NAME  = CamclkUt

SRC_PATH := $(DIR_PATH)

CHIP_DIR=infinity6

KERNEL_DIR ?= $(PWD)/../../../../

INC_PATH := -I$(DIR_PATH)
INC_PATH += -I$(DIR_PATH)/../include
INC_PATH += -I$(DIR_PATH)/../common
INC_PATH += -I$(KERNEL_DIR)/drivers/sstar/include
INC_PATH += -I$(KERNEL_DIR)/drivers/sstar/include/$(CHIP_DIR)


INC_PATH += -I$(DIR_PATH)//drv/inc
INC_PATH += -I$(DIR_PATH)//drv/inc/linux
INC_PATH += -I$(DIR_PATH)//drv/pub
INC_PATH += -I$(DIR_PATH)//hal/common
INC_PATH += -I$(DIR_PATH)//hal/$(CHIP_DIR)/inc
INC_PATH += -I$(DIR_PATH)//hal/$(CHIP_DIR)


EXTRA_CFLAGS += $(INC_PATH)

obj-m       := $(MODULE_NAME).o

$(MODULE_NAME)-objs += .//clk_ut_module.o
$(MODULE_NAME)-objs += .//clk_ut.o

all:
	@echo EXTRA_CFLAGS=$(EXTRA_CFLAGS)
	@echo KERNEL_DIR = $(KERNEL_DIR)
	@echo DIR=$(DIR_PATH)
	make -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	make -C $(KERNEL_DIR) M=$(PWD) clean
