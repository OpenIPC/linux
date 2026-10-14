obj-y			+= chip.o

obj-y			+= ddr.o
ifeq ($(CONFIG_KERNEL_MODE_NEON),y)
obj-y			+= ddr_float.o
CFLAGS_ddr_float.o	+= -ffreestanding
ifeq ($(ARCH),arm)
CFLAGS_ddr_float.o	+= -march=armv7-a -mfloat-abi=softfp -mfpu=neon
endif
ifeq ($(ARCH),arm64)
CFLAGS_REMOVE_ddr_float.o += -mgeneral-regs-only
endif
endif
obj-$(CONFIG_ARM)	+= smp.o
obj-$(CONFIG_PM)	+= pm.o
obj-$(CONFIG_PM)        += pm_lp_entry.o
obj-y			+= dts/

