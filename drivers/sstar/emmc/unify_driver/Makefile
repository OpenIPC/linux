#
# Makefile for MStar eMMC host drivers.
#
obj-$(CONFIG_MS_EMMC)   += kdrv_emmc.o

# general options
EXTRA_CFLAGS    += -I$(srctree)/drivers/sstar/emmc/unify_driver/
EXTRA_CFLAGS    += -I$(srctree)/drivers/sstar/emmc/unify_driver/inc/api/
EXTRA_CFLAGS    += -I$(srctree)/drivers/sstar/emmc/unify_driver/inc/common/
EXTRA_CFLAGS    += -I$(srctree)/drivers/sstar/emmc/unify_driver/inc/config/
EXTRA_CFLAGS    += -I$(srctree)/drivers/sstar/emmc/unify_driver/inc/$(CONFIG_SSTAR_CHIP_NAME)
EXTRA_CFLAGS    += -I$(srctree)/drivers/sstar/include
EXTRA_CFLAGS    += -I$(srctree)/drivers/sstar/include/$(CONFIG_SSTAR_CHIP_NAME)
ifndef CONFIG_ARM_LPAE
#EXTRA_CFLAGS    += -Werror
endif

# specific options

# files
kdrv_emmc-y           += src/api/eMMC_prg.o
kdrv_emmc-y           += src/common/eMMC_hal_speed.o
kdrv_emmc-y           += src/common/eMMC_hal_v5.o
kdrv_emmc-y           += mstar_mci_v5.o
kdrv_emmc-y           += src/common/eMMC_utl.o
kdrv_emmc-y           += src/config/eMMC_platform.o
kdrv_emmc-y           += src/common/eMMC_hal_api.o
kdrv_emmc-y           += src/common/eMMC_hal_command.o
kdrv_emmc-y           += src/$(subst ",,$(CONFIG_SSTAR_CHIP_NAME))/eMMC_platform_config.o
kdrv_emmc-$(CONFIG_MS_EMMC_UT_VERIFY) += src/common/eMMC_ip_verify.o
