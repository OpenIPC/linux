# linux/arch/arm/mach-anycloud/Makefile
#
# Copyright 2020 Anyka Microelectronics
#
# Licensed under GPLv2

# Core support for all Anyka SoCs

# Machin support
obj-$(CONFIG_MACH_AK39EV330)	   += mach-ak39ev330.o
obj-$(CONFIG_MACH_AK39EV330)	   += ak_l2.o

# AK3918EV200 reuses the same machine descriptor (mach-ak39ev330.c handles
# both dt_compat lists and both CPU-ID values, see the EV200 CPU-ID commit)
obj-$(CONFIG_MACH_AK3918EV200)	   += mach-ak39ev330.o
obj-$(CONFIG_MACH_AK3918EV200)	   += ak_l2.o

# RTC raw register helper: shared by drivers/rtc/plat-anyka/rtc.c and
# drivers/watchdog/ak_rtc_wdt.c, neither of which the vendor ships.
# Built for either machine, same as ak_l2.o above.
obj-$(CONFIG_MACH_AK39EV330)	   += ak_rtc.o
obj-$(CONFIG_MACH_AK3918EV200)	   += ak_rtc.o

# SAR-ADC1 raw register helper: shared by drivers/input/keyboard/
# akad-keys.c and drivers/power/plat-anyka/battery.c, same "no vendor
# driver at all" situation as ak_rtc.o above.
obj-$(CONFIG_MACH_AK39EV330)	   += ak_adc1.o
obj-$(CONFIG_MACH_AK3918EV200)	   += ak_adc1.o

# AK3918AV130: the AnyCloud39AV130 SDK's machine and L2 code. Its L2
# controller differs from EV330's, so the vendor's ak_l2.c is kept whole
# as ak_l2_av130.o rather than merged into the EV330 one.
obj-$(CONFIG_MACH_AK3918AV130)	+= mach-ak3918av130.o
obj-$(CONFIG_MACH_AK3918AV130)	+= ak_l2_av130.o
obj-$(CONFIG_MACH_AK3918AV130)	+= ak_module_interface.o
obj-$(CONFIG_MACH_AK3918AV130)	+= ak_cache_count.o
obj-$(CONFIG_MACH_AK3918AV130)	+= ak_notifier.o
obj-$(CONFIG_MACH_AK3918AV130)	+= ak_pm.o
obj-$(CONFIG_MACH_AK3918AV130)	+= ak_l2_exebuf.o
