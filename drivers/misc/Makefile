#
# Makefile for misc devices that really don't fit anywhere else.
#

obj-$(CONFIG_IBM_ASM)		+= ibmasm/
obj-$(CONFIG_AD525X_DPOT)	+= ad525x_dpot.o
obj-$(CONFIG_AD525X_DPOT_I2C)	+= ad525x_dpot-i2c.o
obj-$(CONFIG_AD525X_DPOT_SPI)	+= ad525x_dpot-spi.o
obj-$(CONFIG_INTEL_MID_PTI)	+= pti.o
obj-$(CONFIG_ATMEL_SSC)		+= atmel-ssc.o
obj-$(CONFIG_ATMEL_TCLIB)	+= atmel_tclib.o
obj-$(CONFIG_DUMMY_IRQ)		+= dummy-irq.o
obj-$(CONFIG_ICS932S401)	+= ics932s401.o
obj-$(CONFIG_LKDTM)		+= lkdtm.o
obj-$(CONFIG_TIFM_CORE)       	+= tifm_core.o
obj-$(CONFIG_TIFM_7XX1)       	+= tifm_7xx1.o
obj-$(CONFIG_PHANTOM)		+= phantom.o
obj-$(CONFIG_QCOM_COINCELL)	+= qcom-coincell.o
obj-$(CONFIG_SENSORS_BH1770)	+= bh1770glc.o
obj-$(CONFIG_SENSORS_APDS990X)	+= apds990x.o
obj-$(CONFIG_SGI_IOC4)		+= ioc4.o
obj-$(CONFIG_ENCLOSURE_SERVICES) += enclosure.o
obj-$(CONFIG_KGDB_TESTS)	+= kgdbts.o
obj-$(CONFIG_SGI_XP)		+= sgi-xp/
obj-$(CONFIG_SGI_GRU)		+= sgi-gru/
obj-$(CONFIG_CS5535_MFGPT)	+= cs5535-mfgpt.o
obj-$(CONFIG_HP_ILO)		+= hpilo.o
obj-$(CONFIG_APDS9802ALS)	+= apds9802als.o
obj-$(CONFIG_ISL29003)		+= isl29003.o
obj-$(CONFIG_ISL29020)		+= isl29020.o
obj-$(CONFIG_SENSORS_TSL2550)	+= tsl2550.o
obj-$(CONFIG_DS1682)		+= ds1682.o
obj-$(CONFIG_TI_DAC7512)	+= ti_dac7512.o
obj-$(CONFIG_C2PORT)		+= c2port/
obj-$(CONFIG_HMC6352)		+= hmc6352.o
obj-y				+= eeprom/
obj-y				+= cb710/
obj-$(CONFIG_SPEAR13XX_PCIE_GADGET)	+= spear13xx_pcie_gadget.o
obj-$(CONFIG_VMWARE_BALLOON)	+= vmw_balloon.o
obj-$(CONFIG_ARM_CHARLCD)	+= arm-charlcd.o
obj-$(CONFIG_PCH_PHUB)		+= pch_phub.o
obj-y				+= ti-st/
obj-y				+= lis3lv02d/
obj-$(CONFIG_USB_SWITCH_FSA9480) += fsa9480.o
obj-$(CONFIG_ALTERA_STAPL)	+=altera-stapl/
obj-$(CONFIG_INTEL_MEI)		+= mei/
obj-$(CONFIG_VMWARE_VMCI)	+= vmw_vmci/
obj-$(CONFIG_LATTICE_ECP3_CONFIG)	+= lattice-ecp3-config.o
obj-$(CONFIG_SRAM)		+= sram.o
obj-y				+= mic/
obj-$(CONFIG_GENWQE)		+= genwqe/
obj-$(CONFIG_ECHO)		+= echo/
obj-$(CONFIG_VEXPRESS_SYSCFG)	+= vexpress-syscfg.o
obj-$(CONFIG_CXL_BASE)		+= cxl/
obj-$(CONFIG_PANEL)             += panel.o
ccflags-y += -I$(srctree)/drivers/tee/tee_api

obj-$(CONFIG_FH_DMAC_MISC) += fh_dma_miscdev.o
obj-$(CONFIG_FH_ACW)		+= fh_acw.o
obj-$(CONFIG_FH_DW_I2S)		+= fh_dw_i2s.o
obj-$(CONFIG_FH_SADC_V1)		+= fh_sadc.o
obj-$(CONFIG_FH_SADC_V2)		+= fh_sadc_v2.o
obj-$(CONFIG_FH_SADC_V3)		+= fh_sadc_v3.o
obj-$(CONFIG_FH_EFUSE)		+= fh_efuse.o
obj-$(CONFIG_FH_EFUSE_TEE)		+= fh_efuse_tee.o
obj-$(CONFIG_FH_PINCTRL_MISC_DEV)		+= fh_pinctrl_dev.o
obj-$(CONFIG_FH_CLK_MISC) += fh_clk_miscdev.o
obj-$(CONFIG_FH_PERF_MON) += fh_perf_mon.o
obj-$(CONFIG_FH_STEPMOTOR) += fh_stepmotor.o fh_stepmotor_hw.o
obj-$(CONFIG_FH_TSENSOR) += fh_tsensor.o


lkdtm-$(CONFIG_LKDTM)		+= lkdtm_core.o
lkdtm-$(CONFIG_LKDTM)		+= lkdtm_bugs.o
lkdtm-$(CONFIG_LKDTM)		+= lkdtm_heap.o
lkdtm-$(CONFIG_LKDTM)		+= lkdtm_perms.o
lkdtm-$(CONFIG_LKDTM)		+= lkdtm_rodata_objcopy.o
lkdtm-$(CONFIG_LKDTM)		+= lkdtm_usercopy.o

KCOV_INSTRUMENT_lkdtm_rodata.o	:= n

OBJCOPYFLAGS :=
OBJCOPYFLAGS_lkdtm_rodata_objcopy.o := \
			--set-section-flags .text=alloc,readonly \
			--rename-section .text=.rodata
targets += lkdtm_rodata.o lkdtm_rodata_objcopy.o
$(obj)/lkdtm_rodata_objcopy.o: $(obj)/lkdtm_rodata.o FORCE
	$(call if_changed,objcopy)
