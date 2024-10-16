#
# Makefile for the Allwinner device drivers.
#
ccflags-$(CONFIG_RTL8363NB_VB) += -I$(src)/../../phy/
ccflags-$(CONFIG_RTL8363NB_VB) += -I$(src)/../../phy/rtl8363_nb
obj-$(CONFIG_SUN4I_EMAC) += sun4i-emac.o
obj-$(CONFIG_SUNXI_GMAC) += sunxi_gmac.o
obj-$(CONFIG_SUNXI_GETH) += sunxi_geth.o
sunxi_geth-objs := sunxi-geth.o sunxi-geth-ops.o
sunxi_gmac-objs := sunxi-gmac.o sunxi_gmac_ops.o
