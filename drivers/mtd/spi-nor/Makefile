obj-$(CONFIG_MTD_SPI_NOR)	+= spi-nor.o
obj-$(CONFIG_SPI_FSL_QUADSPI)	+= fsl-quadspi.o
obj-$(CONFIG_SPI_NXP_SPIFI)	+= nxp-spifi.o

# The AK3918AV130 SFC NOR controller.
ifeq ($(CONFIG_MACH_AK3918AV130),y)
obj-$(CONFIG_MTD_AK_SPIFLASH)	+= ak_nor.o
endif
