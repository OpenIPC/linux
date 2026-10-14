ccflags-y += -I$(srctree)/drivers/mtd/nand
obj-y					+= fmc100_host.o
obj-$(CONFIG_LOTUS_FMC100_SPINOR)	+= fmc100_spinor.o
ifneq ($(CONFIG_LOTUS_FMC100_NAND)_$(CONFIG_LOTUS_FMC100_SPINAND),_)
obj-y					+= fmc100_xnand.o
endif

obj-$(CONFIG_ARCH_LOTUS)		+= fmc100_clock.o

