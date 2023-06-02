#
# Makefile for MStar MFE (Multi-Format Encoder) device driver.

ifdef CONFIG_ARCH_INFINITY
    H2IP_NAME := h2v1
    EXTRA_CFLAGS += -DMVHE_MCM_FORCE_OFF
    EXTRA_CFLAGS += -Idrivers/mstar/include/infinity
endif

ifdef CONFIG_ARCH_INFINITY3
    H2IP_NAME := h2v3
endif

# general options
EXTRA_CFLAGS += -Idrivers/mstar/include
EXTRA_CFLAGS += -Idrivers/mstar/mhve
EXTRA_CFLAGS += -Idrivers/mstar/vhe
EXTRA_CFLAGS += -Idrivers/mstar/vhe/$(H2IP_NAME)
#EXTRA_CFLAGS += -DROIMAP

# files
obj-$(CONFIG_MS_VHE) += mdrv_mvhe.o
obj-$(CONFIG_MS_VHE) += $(H2IP_NAME)/

mdrv_mvhe-y :=		\
	mvhe_module.o	\
	mvhe_dev.o		\
	mvhe_ctx.o		\
	mvhe_rqc.o		\
