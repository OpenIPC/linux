#
# Makefile for MStar MFE (Multi-Format Encoder) device driver.

ifdef CONFIG_ARCH_INFINITY
    IP_NAME := mfe5
endif

ifdef CONFIG_ARCH_INFINITY3
    IP_NAME := mfe6
endif

# general options
EXTRA_CFLAGS += -Idrivers/mstar/include
EXTRA_CFLAGS += -Idrivers/mstar/mhve
EXTRA_CFLAGS += -Idrivers/mstar/mfev5
EXTRA_CFLAGS += -Idrivers/mstar/mfev5/$(IP_NAME)

# files
obj-$(CONFIG_MS_MFEV5) += mdrv_mmfe.o
obj-$(CONFIG_MS_MFEV5) += $(IP_NAME)/

mdrv_mmfe-y :=		\
	mmfe_module.o	\
	mmfe_dev.o		\
	mmfe_ctx.o		\
	mmfe_rqc.o		\
