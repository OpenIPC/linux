ccflags-y			+= -I$(srctree)/drivers/sstar/include/$(CONFIG_SSTAR_CHIP_NAME)
ccflags-y                       += -I$(srctree)/drivers/usb/gadget/udc
obj-$(CONFIG_USB_MSB250X) += udc-msb250x.o
udc-msb250x-y :=  msb250x_gadget.o msb250x_dma.o msb250x_ep.o msb250x_udc.o
