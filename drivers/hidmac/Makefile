#
# Makefile for the hi dmac drivers.
#

ifdef CONFIG_HI_DMAC_IO_BASE
obj-$(CONFIG_HI_DMAC)	+= hi_dmac.o
else
# get the io resource from DTS
obj-$(CONFIG_HI_DMAC)	+= hi_pl08x.o
endif
