CONFIG_SSTAR_CHIP_NAME := $(subst ",,$(CONFIG_SSTAR_CHIP_NAME))
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/include
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/include/$(CONFIG_SSTAR_CHIP_NAME)
EXTRA_CFLAGS      +=  -I$(srctree)/arch/arm/include
EXTRA_CFLAGS      +=  -I$(srctree)/drivers/sstar/miu/$(CONFIG_SSTAR_CHIP_NAME)

obj-y += soc.o
obj-$(CONFIG_PM_SLEEP) += pm.o
obj-y += sram.o
