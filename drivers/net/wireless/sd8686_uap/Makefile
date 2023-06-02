EXTRA_CFLAGS += -I$(KERNELDIR)/include
EXTRA_CFLAGS += -DFPNUM='"52"' -DDEBUG_LEVEL1

UAPOBJS = uap_main.o uap_proc.o uap_debug.o uap_sdio_mmc.o
obj-m := uap8xxx.o
uap8xxx-objs := $(UAPOBJS)

