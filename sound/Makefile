# Makefile for the Linux sound card driver
#

obj-$(CONFIG_SOUND) += soundcore.o
obj-$(CONFIG_SOUND_PRIME) += oss/
obj-$(CONFIG_DMASOUND) += oss/
ifneq ($(CONFIG_SND_SUNXI_RPAF),y)
obj-$(CONFIG_SND) += core/ i2c/ drivers/ isa/ pci/ ppc/ arm/ sh/ synth/ usb/ \
	firewire/ sparc/ spi/ parisc/ pcmcia/ mips/ soc/ atmel/ hda/
endif
obj-$(CONFIG_SND_SUNXI_RPAF) += sunxi-rpaf/core/ sunxi-rpaf/soc/ \
	sunxi-rpaf/component/ drivers/ usb/
obj-$(CONFIG_SND_AOA) += aoa/

# This one must be compilable even if sound is configured out
obj-$(CONFIG_AC97_BUS) += ac97_bus.o

ifeq ($(CONFIG_SND),y)
obj-y += last.o
endif
ifeq ($(CONFIG_SND_SUNXI_RPAF), y)
obj-y += last.o
endif

soundcore-objs  := sound_core.o
