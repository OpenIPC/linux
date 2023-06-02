CONFIG_MSTAR_CHIP_NAME := $(subst ",,$(CONFIG_MSTAR_CHIP_NAME))

# general options
EXTRA_CFLAGS      +=  -Idrivers/mstar/flash_isp
EXTRA_CFLAGS      +=  -Idrivers/mstar/flash_isp/include
EXTRA_CFLAGS      +=  -Idrivers/mstar/flash_isp/$(CONFIG_MSTAR_CHIP_NAME)
EXTRA_CFLAGS      +=  -Idrivers/mstar/include

# specific options
EXTRA_CFLAGS      +=  -DMSOS_TYPE_LINUX

# files
obj-$(CONFIG_MS_FLASH_ISP)  += drvSERFLASH.o mtd_serflash.o drvDeviceInfo.o
obj-$(CONFIG_MS_FLASH_ISP)  += $(CONFIG_MSTAR_CHIP_NAME)/halSERFLASH.o
obj-$(CONFIG_MS_FLASH_ISP_MXP_PARTS) += part_mxp.o mxp_flash.o



