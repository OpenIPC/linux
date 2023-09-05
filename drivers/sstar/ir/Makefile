#
# Makefile for the kernel IrDA char device drivers.
#
EXTRA_CFLAGS += -Idrivers/sstar/include
EXTRA_CFLAGS += -Idrivers/sstar/ir
EXTRA_CFLAGS += -Iinclude/linux

obj-$(CONFIG_MS_IR)  += kdrv_ir.o
kdrv_ir-y         += ms_ir.o

