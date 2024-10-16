ccflags-$(CONFIG_RTL8363NB_VB) += -DMDC_MDIO_OPERATION
ccflags-$(CONFIG_RTL8363NB_VB) += -I$(src)
ccflags-$(CONFIG_RTL8363NB_VB) += -I$(src)/../
CONFIG_MODULE_NAME = 8363
8363-$(CONFIG_RTL8363NB_VB) += \
	./rtl8367c_asicdrv_mirror.o \
	./rtl8367c_asicdrv_cputag.o \
	./qos.o \
	./interrupt.o \
	./rtl8367c_asicdrv_eav.o \
	./rtl8367c_asicdrv_i2c.o \
	./rtl8367c_asicdrv_mib.o \
	./rtl8367c_asicdrv_acl.o \
	./smi.o \
	./rtl8367c_asicdrv_dot1x.o \
	./rtl8367c_asicdrv_green.o \
	./rtl8367c_asicdrv_igmp.o \
	./rtl8367c_asicdrv_port.o \
	./trunk.o \
	./oam.o \
	./rtl8367c_asicdrv.o \
	./rtl8367c_asicdrv_svlan.o \
	./eee.o \
	./i2c.o \
	./rtl8367c_asicdrv_lut.o \
	./rtl8367c_asicdrv_interrupt.o \
	./ptp.o \
	./rtl8367c_asicdrv_fc.o \
	./rtl8367c_asicdrv_hsb.o \
	./igmp.o \
	./rtl8367c_asicdrv_vlan.o \
	./rldp.o \
	./rtl8367c_asicdrv_oam.o \
	./rtl8367c_asicdrv_inbwctrl.o \
	./rtl8367c_asicdrv_led.o \
	./rtk_switch.o \
	./rtl8367c_asicdrv_scheduling.o \
	./rtl8367c_asicdrv_storm.o \
	./rtl8367c_asicdrv_meter.o \
	./vlan.o \
	./led.o \
	./mirror.o \
	./rtl8367c_asicdrv_rldp.o \
	./rtl8367c_asicdrv_trunking.o \
	./l2.o \
	./storm.o \
	./cpu.o \
	./rtl8367c_asicdrv_eee.o \
	./dot1x.o \
	./trap.o \
	./rtl8367c_asicdrv_misc.o \
	./svlan.o \
	./rtl8367c_asicdrv_unknownMulticast.o \
	./rtl8367c_asicdrv_phy.o \
	./acl.o \
	./rtl8367c_asicdrv_portIsolation.o \
	./stat.o \
	./port.o \
	./rtl8367c_asicdrv_rma.o \
	./rtl8367c_asicdrv_qos.o \
	./leaky.o \
	./rate.o \

$(CONFIG_MODULE_NAME)-y := $(8363-y)
obj-$(CONFIG_RTL8363NB_VB) += $(CONFIG_MODULE_NAME).o

clean-files += Module.symvers Module.markers modules modules.order

