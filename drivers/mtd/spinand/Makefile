#
# Makefile for the SPI NAND MTD
#

# Core functionality.

obj-$(CONFIG_MTD_SPINAND)	+= spinand.o

# Board specific.
ifdef CONFIG_MTD_SPINAND_GOKE
obj-$(CONFIG_GK_SFLASH_V1_00)	+= gk_spinand_v1_00.o
endif

spinand-objs := spinand_base.o spinand_bbt.o
