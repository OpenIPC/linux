obj-$(CONFIG_MS_SDMMC)   += kdrv_sdmmc.o
kdrv_sdmmc-y =  src/hal_card_regs.o src/hal_card_timer.o

ifeq ($(CONFIG_ARCH_CEDRIC),y)
kdrv_sdmmc-y +=  src/hal_card_intr.o
kdrv_sdmmc-y +=  src/hal_sdmmc.o
kdrv_sdmmc-y +=  src/hal_card_platform_c3.o
else ifeq ($(CONFIG_ARCH_CHICAGO),y)
kdrv_sdmmc-y +=  src/hal_card_intr.o
kdrv_sdmmc-y +=  src/hal_sdmmc.o
kdrv_sdmmc-y +=  src/hal_card_platform_c4.o
else ifeq ($(CONFIG_ARCH_INFINITY),y)
kdrv_sdmmc-y +=  src/hal_card_intr_v5.o
kdrv_sdmmc-y += src/hal_sdmmc_v5.o
kdrv_sdmmc-y += src/hal_card_platform_iNF.o
else ifeq ($(CONFIG_ARCH_INFINITY3),y)
kdrv_sdmmc-y +=  src/hal_card_intr_v5.o
kdrv_sdmmc-y += src/hal_sdmmc_v5.o
kdrv_sdmmc-y += src/hal_card_platform_iNF3.o
endif

kdrv_sdmmc-y += ms_sdmmc_lnx.o
