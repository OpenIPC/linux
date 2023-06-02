#
# Makefile for the linux kernel.

obj-y				+= clock.o

obj-$(CONFIG_PLATFORM_A320)	+= board-a320.o
obj-$(CONFIG_PLATFORM_A369)	+= board-a369.o
obj-$(CONFIG_PLATFORM_AXI)	+= board-axi.o

obj-$(CONFIG_SMP)		+= platsmp.o headsmp.o
obj-$(CONFIG_HOTPLUG_CPU)	+= hotplug.o
obj-$(CONFIG_LOCAL_TIMERS)	+= localtimer.o

obj-$(CONFIG_FTAHBB020)		+= ftahbb020.o
obj-$(CONFIG_FTAPBB020)		+= ftapbb020.o
obj-$(CONFIG_FTDMAC020)		+= ftdmac020.o
obj-$(CONFIG_FTDMAC030)		+= ftdmac030.o
obj-$(CONFIG_FTINTC010)		+= ftintc010.o
obj-$(CONFIG_FTPCI100) 		+= ftpci100.o
obj-$(CONFIG_FTPMU010)		+= ftpmu010.o
obj-$(CONFIG_FTPWMTMR010)	+= ftpwmtmr010.o
obj-$(CONFIG_FTSCU010)		+= ftscu010.o
obj-$(CONFIG_FTTMR010)		+= fttmr010.o
