
CONFIG_MSTAR_CHIP_NAME := $(subst ",,$(CONFIG_MSTAR_CHIP_NAME))
EXTRA_CFLAGS      +=  -Idrivers/mstar/include
EXTRA_CFLAGS      +=  -Idrivers/mstar/include/$(CONFIG_MSTAR_CHIP_NAME)

obj-y += ms_chip.o atags_to_fdt.o
obj-$(CONFIG_TI_CMEM_STUB) += cmemk_stub.o
obj-y += $(CONFIG_MSTAR_CHIP_NAME)/
