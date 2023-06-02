#
# Makefile for NVT processors family
#

obj-$(CONFIG_ARCH_NVT_IVOT_V7)		+= board-dt.o io.o fmem.o

obj-$(CONFIG_NVT_INFO)			+= nvt-info.o

obj-$(CONFIG_SMP)			+= platsmp.o headsmp.o

obj-$(CONFIG_HOTPLUG_CPU)		+= hotplug.o

obj-$(CONFIG_PM)			+= pm.o sleep.o

obj-$(CONFIG_TEE)			+= smc.o