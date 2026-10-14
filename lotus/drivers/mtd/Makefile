
obj-y	+= lotus_flash.o
obj-$(CONFIG_LOTUS_FMC100_SPINOR)	+= spinor_ids.o spinor_driver.o

ifneq ($(CONFIG_LOTUS_FMC100_NAND)_$(CONFIG_LOTUS_FMC100_SPINAND),_)
obj-y	+= spinand_ids.o spinand_driver.o
obj-y	+= readretry/ syncnand/ nand_ids.o
endif

obj-y	+= regop_intf.o flash_cache.o

obj-$(CONFIG_LOTUS_FMC100)	+= fmc100/
