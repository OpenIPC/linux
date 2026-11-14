# linux/arch/arm/mach-anycloud/Makefile
#
# Copyright 2020 Anyka Microelectronics
#
# Licensed under GPLv2

# Core support for all Anyka SoCs

# Machin support
obj-$(CONFIG_MACH_AK37D)	   += mach-ak37d.o
obj-$(CONFIG_MACH_AK37D)	   += ak_l2.o
obj-$(CONFIG_MACH_AK37D)	+= ak_module_interface.o

obj-$(CONFIG_MACH_AK39EV330)	   += mach-ak39ev330.o
obj-$(CONFIG_MACH_AK39EV330)	   += ak_l2.o
obj-$(CONFIG_MACH_AK39EV330)	+= ak_module_interface.o

obj-$(CONFIG_MACH_AK37E)	+= mach-ak37e.o
obj-$(CONFIG_MACH_AK37E)	+= ak_l2_dual.o
obj-$(CONFIG_MACH_AK37E)	+= ak_module_interface.o


obj-$(CONFIG_MACH_AK3918AV100)	+= mach-ak3918av100.o
obj-$(CONFIG_MACH_AK3918AV100)	+= ak_l2_dual.o
obj-$(CONFIG_MACH_AK3918AV100)	+= ak_module_interface.o
obj-$(CONFIG_MACH_AK3918AV100)	+= ak_cache_count.o

obj-$(CONFIG_MACH_AK3918EV300L)	+= mach-ak3918ev300l.o
obj-$(CONFIG_MACH_AK3918EV300L)	+= ak_l2_dual.o
obj-$(CONFIG_MACH_AK3918EV300L)	+= ak_module_interface.o
obj-$(CONFIG_MACH_AK3918EV300L)	+= ak_cache_count.o

obj-y	+= ak_notifier.o

obj-$(CONFIG_MACH_AK3918AV100)	+= ak_pm.o
obj-$(CONFIG_MACH_AK3918AV100)	+= ak_l2_exebuf.o
obj-$(CONFIG_MACH_AK3918EV300L)	+= ak_pm.o
obj-$(CONFIG_MACH_AK3918EV300L)	+= ak_l2_exebuf.o
