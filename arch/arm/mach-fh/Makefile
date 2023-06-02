#
# Makefile for the linux kernel.
#
#

# Common objects
obj-y 			:= time.o clock.o \
			   sram.o irq.o pmu.o pm.o sram.o
# gpio.o
# Chip specific
obj-$(CONFIG_ARCH_FH8833)       += fh8833.o
# Board specific
obj-$(CONFIG_MACH_FH8833) 	+= board-fh8833.o pinctrl.o
obj-$(CONFIG_FH_SIMPLE_TIMER) 	+= fh_simple_timer.o

# Power Management
obj-$(CONFIG_CPU_FREQ)		+= cpufreq.o
obj-$(CONFIG_CPU_IDLE)		+= cpuidle.o
obj-$(CONFIG_SUSPEND)		+= pm.o sleep.o
