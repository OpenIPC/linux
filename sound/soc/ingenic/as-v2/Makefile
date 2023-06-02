ccflags-$(CONFIG_SND_ASOC_INGENIC_DEBUG) += -DDEBUG
ccflags-$(CONFIG_SND_ASOC_INGENIC_VERBOSE) += -DVERBOSE_DEBUG

snd-asoc-as-fe-objs 		:= as-dma.o as-dsp.o as-mixer.o as-fmtcov.o
snd-asoc-as-be-baic-objs	:= as-baic.o as-spdif.o as-dmic.o

obj-$(CONFIG_SND_ASOC_INGENIC_AS_FE) += snd-asoc-as-fe.o
obj-$(CONFIG_SND_ASOC_INGENIC_AS_BAIC) += snd-asoc-as-be-baic.o

obj-$(CONFIG_SND_ASOC_INGENIC_AS_VIR_FE) += as-vir-fe.o
