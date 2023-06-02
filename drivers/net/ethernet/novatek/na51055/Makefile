#
# Makefile for the Synopsys network device drivers.
#

obj-$(CONFIG_NOVATEK_DWC_ETH_QOS_NA51055) += ntkimethmac.o
ifeq ($(CONFIG_NVT_MDIO_NA51055_CFG),y)
ntkimethmac-objs := DWC_ETH_QOS.o \
			DWC_ETH_QOS_dev.o DWC_ETH_QOS_desc.o \
			DWC_ETH_QOS_drv.o DWC_ETH_QOS_ethtool.o \
			DWC_ETH_QOS_debug_operation.o


else
ntkimethmac-objs := DWC_ETH_QOS.o DWC_ETH_QOS_mdio.o\
                        DWC_ETH_QOS_dev.o DWC_ETH_QOS_desc.o \
                        DWC_ETH_QOS_drv.o DWC_ETH_QOS_ethtool.o \
                        DWC_ETH_QOS_debug_operation.o
endif

obj-$(CONFIG_NVT_MDIO_NA51055)   += DWC_ETH_QOS_mdio_driver.o
