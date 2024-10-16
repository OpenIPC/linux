obj-$(CONFIG_SND_SUNXI_HIFI) += rpmsg_hifi.o

# SUNXI Platform Support
snd-soc-sunxi-hifi-objs := sunxi-hifi-pcm.o
snd-soc-sunxi-hifi-daudio-objs := sunxi-daudio.o
snd-soc-sunxi-hifi-dmic-objs := sunxi-dmic.o

obj-$(CONFIG_SND_SUNXI_SOC_HIFI) += snd-soc-sunxi-hifi.o
obj-$(CONFIG_SND_SUNXI_SOC_HIFI_DAUDIO) += snd-soc-sunxi-hifi-daudio.o
obj-$(CONFIG_SND_SUNXI_SOC_HIFI_DMIC) += snd-soc-sunxi-hifi-dmic.o

# SUNXI Codec Embedded Support
obj-$(CONFIG_SND_SUN50IW11_SOC_HIFI_CODEC) += sun50iw11-codec.o
obj-$(CONFIG_SND_SUN50IW11_SOC_HIFI_CPUDAI) += sun50iw11-cpudai.o

# SUNXI Machine Support
obj-$(CONFIG_SND_SUN50IW11_HIFI_CODEC) += sun50iw11-sndcodec.o
snd-sunxi-hifi-snddaudio-objs := sunxi-snddaudio.o
obj-$(CONFIG_SND_SUNXI_HIFI_DAUDIO) += snd-sunxi-hifi-snddaudio.o
snd-sunxi-hifi-snddmic-objs := sunxi-snddmic.o
obj-$(CONFIG_SND_SUNXI_HIFI_DMIC) += snd-sunxi-hifi-snddmic.o
