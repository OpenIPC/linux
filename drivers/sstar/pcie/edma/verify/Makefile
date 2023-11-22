#
# Makefile for kernel test
#
PWD         ?= $(shell pwd)
DIR_PATH    := $(PWD)/..
KVERSION    := $(shell uname -r)

MODULE_NAME  = edma_ut

SRC_PATH := $(DIR_PATH)

KERNEL_DIR ?= $(PWD)/../../../../..

INC_PATH += -I$(DIR_PATH)/drv/pub
INC_PATH += -I$(KERNEL_DIR)/drivers/sstar/include
INC_PATH += -I$(KERNEL_DIR)/drivers/pci/controller/dwc

EXTRA_CFLAGS += $(INC_PATH)

obj-m       := $(MODULE_NAME).o

$(MODULE_NAME)-objs += edma_ut_module.o

all:
	@echo EXTRA_CFLAGS=$(EXTRA_CFLAGS)
	@echo KERNEL_DIR = $(KERNEL_DIR)
	@echo DIR=$(DIR_PATH)
	make -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	make -C $(KERNEL_DIR) M=$(PWD) clean
