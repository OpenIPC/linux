CONFIG_SSTAR_CHIP_NAME := $(subst ",,$(CONFIG_SSTAR_CHIP_NAME))
EXTRA_CFLAGS      += -I$(srctree)/drivers/sstar/include
EXTRA_CFLAGS      += -I$(srctree)/drivers/sstar/include/$(CONFIG_SSTAR_CHIP_NAME)

obj-$(CONFIG_SS_VOLTAGE_IDAC_CTRL) += voltage_request_init.o
