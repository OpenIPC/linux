CONFIG_SSTAR_CHIP_NAME := $(subst ",,$(CONFIG_SSTAR_CHIP_NAME))

# general options
EXTRA_CFLAGS      +=  -Idrivers/sstar/include
EXTRA_CFLAGS      +=  -Idrivers/sstar/include/$(CONFIG_SSTAR_CHIP_NAME)
EXTRA_CFLAGS      +=  -Idrivers/sstar/crypto/infinity
EXTRA_CFLAGS      +=  -Idrivers/sstar/bdma/
EXTRA_CFLAGS      +=  -Idrivers/sstar/movedma/

obj-$(CONFIG_MS_MSYS) = ms_msys.o
obj-$(CONFIG_MS_MSYS) += $(CONFIG_SSTAR_CHIP_NAME)/platform_msys.o
obj-$(CONFIG_MS_MSYS_LOG) += ms_msys_log.o