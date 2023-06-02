#
# Makefile for the linux kernel.

# obj-y += xxx.o
obj-y += ftpmu010.o fttmr010.o fmem.o gm_jiffies.o
obj-$(CONFIG_FTDMAC020) += ftdmac020.o dma_gm.o
obj-$(CONFIG_FTAPBB020) += ftapbb020.o dma_gm.o

obj-$(CONFIG_PLATFORM_GM8126)   += ftintc010.o
obj-$(CONFIG_PLATFORM_GM8287)   += ftintc030.o
obj-$(CONFIG_PLATFORM_GM8139)   += ftintc030.o
obj-$(CONFIG_PLATFORM_GM8136)   += ftintc030.o
GM-platform-$(CONFIG_PLATFORM_GM8126)       := platform-GM8126
GM-platform-$(CONFIG_PLATFORM_GM8287)       := platform-GM8287
GM-platform-$(CONFIG_PLATFORM_GM8139)       := platform-GM8139
GM-platform-$(CONFIG_PLATFORM_GM8136)       := platform-GM8136

ifeq ($(CONFIG_CPU_FREQ),y)
obj-$(CONFIG_PLATFORM_GM8139)		+= cpufreq.o
obj-$(CONFIG_PLATFORM_GM8136)		+= cpufreq.o
endif

#
#Default platform directory set to GM8181
#TODO: Make this an error, should never happen unless the Kconfig or Makefile is wrong
#
ifeq ($(GM-platform-y),)
GM-platform-y := platform-GM8126
endif

PLATFORM_DIR   := $(GM-platform-y)

core-y      +=  arch/arm/mach-GM/$(PLATFORM_DIR)/

define create-platform-symlink
  mkdir -p arch/arm/mach-GM/include/mach;                 \
  if [ -L $@ ]; then                              \
    platformlink=`readlink $@`;                     \
  fi;												\
  if [ ! -e $@ ] || [ $$platformlink != $(PLATFORM_DIR) ]; then           \
	 touch arch/arm/mach-GM/include/mach/$(PLATFORM_DIR)/*;      \
  fi;															\
  echo '  SYMLINK $@ -> arch/arm/mach-GM/include/mach/$(PLATFORM_DIR)';   \
  ln -fsn $(PLATFORM_DIR) $@;
endef

arch/arm/mach-GM/include/mach/platform: FORCE
	 $(Q)$(create-platform-symlink)

prepare: arch/arm/mach-GM/include/mach/platform	 


