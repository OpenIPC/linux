# SUNXI Platform Support
snd-soc-sunxi-objs := sunxi-pcm.o
snd-soc-sunxi-daudio-objs := sunxi-daudio.o
snd-soc-sunxi-spdif-objs := sunxi-spdif.o

obj-$(CONFIG_SND_SUNXI_SOC) += snd-soc-sunxi.o
obj-$(CONFIG_SND_SUNXI_SOC_DAUDIO) += snd-soc-sunxi-daudio.o
obj-$(CONFIG_SND_SUNXI_SOC_SPDIF) += snd-soc-sunxi-spdif.o spdif-utils.o

# SUNXI Machine Support
obj-$(CONFIG_SND_SUNXI_DAUDIO) += sunxi-snddaudio.o
obj-$(CONFIG_SND_SUNXI_SPDIF) += sunxi-sndspdif.o
