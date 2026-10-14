
ccflags-y += -I$(srctree)/drivers/mmc/host/

obj-$(CONFIG_MMC_SDHCI_LOTUS)		+= sdhci-lotus.o
sdhci-lotus-soc-$(CONFIG_ARCH_XMFALCON) += sdhci-xmfalcon.o
sdhci-lotus-soc-$(CONFIG_ARCH_XMORCA) += sdhci-xmorca.o
sdhci-lotus-objs := $(sdhci-lotus-soc-y)
