CONFIG_SSTAR_CHIP_NAME := $(subst ",,$(CONFIG_SSTAR_CHIP_NAME))
EXTRA_CFLAGS      +=  -Idrivers/sstar/include
EXTRA_CFLAGS      +=  -Idrivers/sstar/include/$(CONFIG_SSTAR_CHIP_NAME)
ifneq ($(CONFIG_SS_USB_LOWPWR_SUSPEND),)
EXTRA_CFLAGS      +=  -Idrivers/usb/dwc3
endif

obj-y += soc.o
obj-$(CONFIG_PM_SLEEP) += pm.o
obj-y += sram.o
obj-y += miu_bw.o
obj-$(CONFIG_SMP) += smp_platform.o smp_head.o
obj-$(CONFIG_SS_MIU_ARBITRATION) += miu_arb.o
