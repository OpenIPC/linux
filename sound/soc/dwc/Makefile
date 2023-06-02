# AT91 Platform Support
snd-soc-fullhan-pcm-objs := fullhan-pcm.o
#snd-soc-fullhan_ssc_dai-objs := fh_i2s_dai.o

obj-$(CONFIG_SND_FULLHAN_SOC) += snd-soc-fullhan-pcm.o
#obj-$(CONFIG_SND_FULLHAN_SOC_SSC) += snd-soc-fullhan_ssc_dai.o

# AT91 Machine Support
snd-soc-sam9g20-wm8731-objs := sam9g20_wm8731.o

# AT32 Machine Support
snd-soc-playpaq-objs := playpaq_wm8510.o
snd-soc-fh-objs := fh.o dma.o
obj-$(CONFIG_SND_FH_SOC_I2S) += snd-soc-fh-i2s.o
snd-soc-fh-i2s-objs := fh_i2s.o


obj-$(CONFIG_SND_FULLHAN_SOC_SAM9G20_WM8731) += snd-soc-sam9g20-wm8731.o
obj-$(CONFIG_SND_FULLHAN_SOC_PLAYPAQ) += snd-soc-playpaq.o
obj-$(CONFIG_SND_FULLHAN_SOC_FH) += snd-soc-fh.o
