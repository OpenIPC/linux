
obj-y	+= xmedia_flash.o
obj-$(CONFIG_MTD_XMEDIA_FMC100_SPINOR)	+= spinor_ids.o spinor_driver.o

ifneq ($(CONFIG_MTD_XMEDIA_FMC100_NAND)_$(CONFIG_MTD_XMEDIA_FMC100_SPINAND),_)
obj-y	+= spinand_ids.o spinand_driver.o
obj-y	+= readretry/ syncnand/ nand_ids.o
endif

obj-y	+= regop_intf.o flash_cache.o

obj-$(CONFIG_MTD_XMEDIA_FMC100)	+= xmedia_fmc100/
