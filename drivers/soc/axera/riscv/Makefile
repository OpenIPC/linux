MKFILE_PATH        := $(abspath $(lastword $(MAKEFILE_LIST)))
HOME_PATH          := $(abspath $(dir $(MKFILE_PATH))../../../../../../..)

EXTRA_CFLAGS += -I. \
		-I$(HOME_PATH)/kernel/osal/include

ccflags-y += -Werror -Wall
obj-$(CONFIG_AXERA_RISCV_DRV) += ax_riscv_drv.o ax_riscv_utils.o ax_riscv_debug.o
