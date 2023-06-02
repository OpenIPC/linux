#
# Makefile for MStar eMMC host drivers.
#
obj-$(CONFIG_MS_EMMC)   += kdrv_emmc.o

# general options
EXTRA_CFLAGS    += -Idrivers/mstar/emmc/unify_driver/inc/api/
EXTRA_CFLAGS    += -Idrivers/mstar/emmc/unify_driver/inc/common/
EXTRA_CFLAGS    += -Idrivers/mstar/emmc/unify_driver/inc/config/
EXTRA_CFLAGS	+= -Idrivers/mstar/include
ifndef CONFIG_ARM_LPAE
#EXTRA_CFLAGS    += -Werror
endif

# specific options

# files
kdrv_emmc-y           += src/api/eMMC_prg.o
kdrv_emmc-y           += src/common/eMMC_hal_speed.o
kdrv_emmc-y           += src/common/eMMC_hal_v5.o
kdrv_emmc-y           += mstar_mci_v5.o

kdrv_emmc-y           += src/common/eMMC_ip_verify.o
kdrv_emmc-y           += src/common/eMMC_utl.o
kdrv_emmc-y           += src/config/eMMC_platform.o

