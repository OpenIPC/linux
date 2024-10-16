obj-$(CONFIG_SUNXI_NAND) += nand.o

nftl=nftl.S

$(obj)/$(nftl) : FORCE

ifneq ($(wildcard $(obj)/lib),)
	$(MAKE) -C $(obj)/.. LICHEE_KDIR="$(shell pwd)" build
endif

nand-objs += nand_interface.o \
	nfd/nand_ota_burn.o \
	nfd/nand_base.o \
	nfd/nand_blk.o \
	nfd/nand_dev.o \
	nfd/nand_class.o \
	nfd/nand_test.o \
	nfd/nand_osal_for_linux.o \
	nfd/nand_lib_cfg.o \
	phy-nand/phy/build_nand_partition.o \
	phy-nand/phy/build_phy_partition.o \
	phy-nand/phy/nand_info_init.o \
	phy-nand/physic_v2/chip/nand_chip_common.o \
	phy-nand/physic_v2/chip/nand_chip_function.o \
	phy-nand/physic_v2/chip/nand_chip_interface.o \
	phy-nand/physic_v2/chip/nand_super_chip_common.o \
	phy-nand/physic_v2/chip/nand_super_chip_function.o \
	phy-nand/physic_v2/chip/nand_super_chip_interface.o \
	phy-nand/physic_v2/controller/nand_controller.o \
	phy-nand/physic_v2/controller/nand_controller_common.o \
	phy-nand/physic_v2/controller/nand_controller_op.o \
	phy-nand/physic_v2/hynix16nm/nand1_chip_for_boot.o \
	phy-nand/physic_v2/hynix16nm_4G/nand8_chip_for_boot.o \
	phy-nand/physic_v2/hynix20nm/nand2_chip_for_boot.o \
	phy-nand/physic_v2/hynix26nm/nand3_chip_for_boot.o \
	phy-nand/physic_v2/universal/nand0_chip_for_boot.o \
	phy-nand/physic_v2/universal/nand0_chip_op.o \
	phy-nand/physic_v2/universal/nand0_super_chip_op.o \
	phy-nand/physic_v2/nand_id2.o \
	phy-nand/physic_v2/nand_phy_init.o \
	phy-nand/physic_v2/nand_phyisc_interface.o \
	phy-nand/physic_v2/nand_platform_data.o \
	phy-nand/physic_v2/nand_secure_storage.o \
	phy-nand/physic_v2/uboot_tail.o \
	phy-nand/physic_v2/version.o \
	phy-nand/phy_v2/build_phy_partition_v2.o \
	phy-nand/phy_v2/nand_info_init_v2.o \
	nftl.o

ifneq ($(CONFIG_SUNXI_PANICPART),)
nand-objs += nfd/nand_panic.o
endif
