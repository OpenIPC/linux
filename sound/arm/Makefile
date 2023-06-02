#
# Makefile for ALSA
#

obj-$(CONFIG_SND_ARMAACI)	+= snd-aaci.o
snd-aaci-objs			:= aaci.o

obj-$(CONFIG_SND_PXA2XX_PCM)	+= snd-pxa2xx-pcm.o
snd-pxa2xx-pcm-objs		:= pxa2xx-pcm.o
obj-$(CONFIG_SND_FH_PCM)	+= snd-fh-pcm.o
snd-fh-pcm-objs		:= fh_pcm.o

obj-$(CONFIG_SND_PXA2XX_LIB)	+= snd-pxa2xx-lib.o
obj-$(CONFIG_SND_FH_LIB)	+= snd-fh-lib.o
snd-pxa2xx-lib-y		:= pxa2xx-pcm-lib.o
snd-fh-lib-y		:= fh-pcm-lib.o
snd-pxa2xx-lib-$(CONFIG_SND_PXA2XX_LIB_AC97)	+= pxa2xx-ac97-lib.o
snd-fh-lib-$(CONFIG_SND_FH_LIB_AC97)	+= fh-ac97-lib.o
obj-$(CONFIG_SND_PXA2XX_AC97)	+= snd-pxa2xx-ac97.o
obj-$(CONFIG_SND_FH_AC97)	+= snd-fh-ac97.o
snd-pxa2xx-ac97-objs		:= pxa2xx-ac97.o
snd-fh-ac97-objs		:= fh-ac97.o