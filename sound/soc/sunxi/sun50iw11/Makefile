# SUNXI Platform Support
snd-soc-sunxi-objs := sunxi-pcm.o
snd-soc-sunxi-daudio-objs := sunxi-daudio.o
snd-soc-sunxi-spdif-objs := sunxi-spdif.o
snd-soc-sunxi-dmic-objs := sunxi-dmic.o
snd-soc-sunxi-cpudai-objs := sunxi-cpudai.o

obj-$(CONFIG_SND_SUN50IW11_SOC) += snd-soc-sunxi.o
obj-$(CONFIG_SND_SUN50IW11_SOC_DAUDIO) += snd-soc-sunxi-daudio.o
obj-$(CONFIG_SND_SUN50IW11_SOC_SPDIF) += snd-soc-sunxi-spdif.o spdif-utils.o
obj-$(CONFIG_SND_SUN50IW11_SOC_DMIC) += snd-soc-sunxi-dmic.o
obj-$(CONFIG_SND_SUN50IW11_SOC_CPUDAI) += snd-soc-sunxi-cpudai.o

# SUNXI Codec Embedded Support
obj-$(CONFIG_SND_SUN50IW11_CODEC) += sunxi-codec.o

# SUNXI Machine Support
obj-$(CONFIG_SND_SUNXI_SOC_SUN50IW11_CODEC) += sunxi-sndcodec.o

obj-$(CONFIG_SND_SUNXI_SOC_SUN50IW11_DAUDIO) += sunxi-snddaudio.o
obj-$(CONFIG_SND_SUNXI_SOC_SUN50IW11_SPDIF) += sunxi-sndspdif.o
obj-$(CONFIG_SND_SUNXI_SOC_SUN50IW11_DMIC) += sunxi-snddmic.o

#MAD module
obj-$(CONFIG_SND_SUN50IW11_SOC_MAD) += sunxi-mad.o
