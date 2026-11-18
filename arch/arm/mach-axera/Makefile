# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the linux kernel.
#
obj-$(CONFIG_ARCH_AXERA) += axera_chip.o
obj-$(CONFIG_PM_SLEEP) += pm.o
ifeq ($(CONFIG_PM_SLEEP),y)
obj-$(CONFIG_ARCH_AX620E) +=  ax620e_sleep.o ax620e_pm.o
endif
obj-$(CONFIG_SMP)			+= platsmp.o headsmp.o
