ifneq ($(CONFIG_SND_SOC_NVT),)
snd-soc-nvt-embd-r-objs	:= na51055_soc_embd_r.o eac.o
snd-soc-nvt-embd-p-objs	:= na51055_soc_embd_p.o eac.o
snd-soc-nvt-embd-l-objs	:= na51055_soc_embd_l.o eac.o
snd-soc-nvt-plat-r-objs	:= na51055_soc_plat_r.o dai.o
snd-soc-nvt-plat-p-objs	:= na51055_soc_plat_p.o dai.o
snd-soc-nvt-plat-l-objs	:= na51055_soc_plat_l.o dai.o
snd-soc-nvt-objs    := nvt_soc_machine.o

obj-$(CONFIG_SND_SOC_NVT) += snd-soc-nvt.o
obj-$(CONFIG_SND_SOC_NVT) += snd-soc-nvt-plat-r.o
obj-$(CONFIG_SND_SOC_NVT) += snd-soc-nvt-plat-p.o
obj-$(CONFIG_SND_SOC_NVT) += snd-soc-nvt-plat-l.o
obj-$(CONFIG_SND_SOC_NVT) += snd-soc-nvt-embd-r.o
obj-$(CONFIG_SND_SOC_NVT) += snd-soc-nvt-embd-p.o
obj-$(CONFIG_SND_SOC_NVT) += snd-soc-nvt-embd-l.o
else ifneq ($(CONFIG_SND_SOC_XMOS),)
snd-soc-nvt-plat-r-objs        := na51055_soc_plat_r.o dai.o
snd-soc-xmos-ext-objs   := xmos_soc_ext.o
snd-soc-xmos-objs      := xmos_soc_machine.o
snd-soc-nvt-embd-p-objs        := na51055_soc_embd_p.o eac.o
snd-soc-nvt-plat-p-objs        := na51055_soc_plat_p.o dai.o
obj-$(CONFIG_SND_SOC_XMOS) += snd-soc-nvt-plat-p.o
obj-$(CONFIG_SND_SOC_XMOS) += snd-soc-nvt-plat-r.o
obj-$(CONFIG_SND_SOC_XMOS) += snd-soc-nvt-embd-p.o
obj-$(CONFIG_SND_SOC_XMOS) += snd-soc-xmos.o
obj-$(CONFIG_SND_SOC_XMOS) += snd-soc-xmos-ext.o
endif
