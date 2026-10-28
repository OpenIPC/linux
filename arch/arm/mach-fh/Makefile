#
# Makefile for the linux kernel.
#
#

# Common objects
obj-y		:= time.o \
			sram.o irq.o pmu.o pm.o sram.o fh_common.o fh_chipid.o
# clk config
obj-$(CONFIG_ARCH_FH8856)       += clock.o 
obj-$(CONFIG_ARCH_FH8833)       += clock.o
# Chip specific
obj-$(CONFIG_ARCH_FH8856)       += fh8856.o
obj-$(CONFIG_ARCH_FH8833)       += fh8833.o
# Board specific
obj-$(CONFIG_ARCH_FH8856) 	+= board-fh8856.o pinctrl.o
obj-$(CONFIG_MACH_FH8833) 	+= board-fh8833.o pinctrl-fh8833.o
obj-$(CONFIG_FH_SIMPLE_TIMER) 	+= fh_simple_timer.o

# Power Management
obj-$(CONFIG_CPU_FREQ)		+= cpufreq.o
obj-$(CONFIG_CPU_IDLE)		+= cpuidle.o
obj-$(CONFIG_SUSPEND)		+= pm.o sleep.o
