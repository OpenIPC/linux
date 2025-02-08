
obj-y					+= xmedia_fmc100_host.o
obj-$(CONFIG_MTD_XMEDIA_FMC100_SPINOR)	+= xmedia_fmc100_spinor.o
ifneq ($(CONFIG_MTD_XMEDIA_FMC100_NAND)_$(CONFIG_MTD_XMEDIA_FMC100_SPINAND),_)
obj-y					+= xmedia_fmc100_xnand.o
endif

obj-$(CONFIG_ARCH_XMEDIA)		+= xmedia_fmc100_clock.o

ccflags-y += -I./drivers/mtd/xmedia
