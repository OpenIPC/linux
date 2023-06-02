CONFIG_FH_CHIP_NAME 	:= $(subst ",,$(CONFIG_FH_CHIP_NAME))
EXTRA_CFLAGS      	+= -Iarch/arm/mach-fh/$(CONFIG_FH_CHIP_NAME)

obj-y	+= pmu.o fullhan.o fh_common.o fh_chipid.o

ifneq ($(CONFIG_USE_OF),y)
obj-y 			+= pinctrl.o clock.o
obj-y += $(CONFIG_FH_CHIP_NAME)/
endif

obj-$(CONFIG_SMP)		+= platsmp.o hotplug.o
obj-$(CONFIG_ARM_FULLHAN_CPUIDLE)		+= cpuidle.o
