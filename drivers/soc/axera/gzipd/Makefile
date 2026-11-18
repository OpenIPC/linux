MKFILE_PATH        := $(abspath $(lastword $(MAKEFILE_LIST)))
HOME_PATH          := $(abspath $(dir $(MKFILE_PATH))../../../../../../..)
# BUILD_PATH         := $(HOME_PATH)/build
KDIR               := $(abspath $(dir $(MKFILE_PATH))../../../../)

EXTRA_CFLAGS += -I. \
		-I$(HOME_PATH)/header/external \
		-I$(HOME_PATH)/msp/out/$(ARCH)_$(LIBC)/include \
		-I$(HOME_PATH)/msp/component/drv/modules/gzipd \
		-I$(KDIR)/include/linux \
		-I$(KDIR)/arch/arm64/include/asm \

ccflags-y += -Werror -Wall
obj-$(CONFIG_ARCH_AXERA) += ax_gzipd_drv.o ax_gzipd_adapter.o ax_gzipd_mng.o
