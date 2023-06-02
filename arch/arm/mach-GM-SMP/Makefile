#
# Makefile for the linux kernel.

# obj-y += xxx.o
obj-y += ftpmu010.o fttmr010.o fmem.o dma_gm.o gm_jiffies.o clock.o
obj-$(CONFIG_FTDMAC020) += ftdmac020.o
obj-$(CONFIG_FTDMAC030) += ftdmac030.o
obj-$(CONFIG_FTAPBB020) += ftapbb020.o
obj-$(CONFIG_SMP)		+= platsmp.o headsmp.o
obj-$(CONFIG_HOTPLUG_CPU)	+= hotplug.o

GM-platform-$(CONFIG_PLATFORM_GM8220)       := platform-GM8220

ifeq ($(CONFIG_CPU_FREQ),y)
obj-$(CONFIG_PLATFORM_GM8220)		+= cpufreq.o
endif

#
#Default platform directory set to GM8220
#TODO: Make this an error, should never happen unless the Kconfig or Makefile is wrong
#
ifeq ($(GM-platform-y),)
GM-platform-y := platform-GM8220
endif

PLATFORM_DIR   := $(GM-platform-y)

core-y      +=  arch/arm/mach-GM-SMP/$(PLATFORM_DIR)/

define create-platform-symlink
  mkdir -p arch/arm/mach-GM-SMP/include/mach;                 \
  if [ -L $@ ]; then                              \
    platformlink=`readlink $@`;                     \
  fi;												\
  if [ ! -e $@ ] || [ $$platformlink != $(PLATFORM_DIR) ]; then           \
	 touch arch/arm/mach-GM-SMP/include/mach/$(PLATFORM_DIR)/*;      \
  fi;															\
  echo '  SYMLINK $@ -> arch/arm/mach-GM-SMP/include/mach/$(PLATFORM_DIR)';   \
  ln -fsn $(PLATFORM_DIR) $@;
endef

arch/arm/mach-GM-SMP/include/mach/platform: FORCE
	 $(Q)$(create-platform-symlink)

prepare: arch/arm/mach-GM-SMP/include/mach/platform	 


