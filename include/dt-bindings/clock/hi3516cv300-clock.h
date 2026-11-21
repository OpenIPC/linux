/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2016 HiSilicon Technologies Co., Ltd.
 *
 * Clock identifiers of the CRG and system controller of the Hi3516C V300
 * and the Hi3516E V100 (ARM926EJ-S / ARMv5TE). Both SoCs have the same CRG
 * register description; see hisilicon,hi3516cv300-crg.yaml.
 *
 * The identifiers that the vendor device trees use keep their values, except
 * the removed ones listed below, and the file name and the HI3516CV300_
 * prefix are the ones the device trees and the clock driver already use.
 * Reset specifiers are the raw register offset and bit number of a soft
 * reset bit that the data sheet lists; the CRG driver rejects any other.
 */

#ifndef __DTS_HI3516CV300_CLOCK_H
#define __DTS_HI3516CV300_CLOCK_H

/*
 * Identifiers 3, 4, 9, 14, 20, 23 and 26 to 28 (fixed 12, 15, 44, 75, 150,
 * 200, 300, 396 and 400 MHz clocks that no select of the CRG offers), 61
 * (Ethernet MACIF gate, there is no such bit) and 64 (USB2 gate, it sat at
 * a soft reset bit) were defined by earlier versions of this header. No
 * working clock was ever behind them, so they are removed instead of being
 * kept as names that return an error: a device tree that still uses one
 * fails to build. The numbers are not reused, and neither are the unused
 * HI3516CV300_CRG_NR_RSTS and HI3516CV300_SYS_NR_RSTS.
 */

/* fixed rate clocks */
#define HI3516CV300_FIXED_3M		1
#define HI3516CV300_FIXED_6M		2
#define HI3516CV300_FIXED_24M		5
#define HI3516CV300_FIXED_25M		6
#define HI3516CV300_FIXED_27M		7
#define HI3516CV300_FIXED_37P125M	8
#define HI3516CV300_FIXED_49P5M		10
#define HI3516CV300_FIXED_50M		11
#define HI3516CV300_FIXED_54M		12
#define HI3516CV300_FIXED_74P25M	13
#define HI3516CV300_FIXED_83P3M		15
#define HI3516CV300_FIXED_99M		16
#define HI3516CV300_FIXED_100M		17
#define HI3516CV300_FIXED_125M		18
#define HI3516CV300_FIXED_148P5M	19
#define HI3516CV300_FIXED_166P6M	21
#define HI3516CV300_FIXED_198M		22
#define HI3516CV300_FIXED_250M		24
#define HI3516CV300_FIXED_297M		25
/* RMII reference clock input of the Ethernet pad, 50 MHz */
#define HI3516CV300_FIXED_RMII_PAD	29

/*
 * mux clocks (parent of gates); MMC0 and MMC1 are SDIO0 and SDIO1, MMC2 is
 * the eMMC controller and MMC3 is SDIO2
 */
#define HI3516CV300_APB_CLK		30
#define HI3516CV300_UART_MUX		31
#define HI3516CV300_FMC_MUX		32
#define HI3516CV300_MMC0_MUX		33
#define HI3516CV300_MMC1_MUX		34
#define HI3516CV300_MMC2_MUX		35
#define HI3516CV300_MMC3_MUX		36
#define HI3516CV300_SENSOR_MUX		37
#define HI3516CV300_VIU_MUX		38
#define HI3516CV300_VEDU_MUX		39
#define HI3516CV300_VPSS_MUX		40
#define HI3516CV300_VGS_MUX		41
#define HI3516CV300_IVE_MUX		42
#define HI3516CV300_PWM_MUX		43
#define HI3516CV300_ETH_CORE_MUX	44
#define HI3516CV300_CIPHER_MUX		46
#define HI3516CV300_ETH_RMII_MUX	47
#define HI3516CV300_ETH_PHY_MUX		48

/* divider clocks */
#define HI3516CV300_ISP_DIV		45

/* gate clocks */
#define HI3516CV300_UART0_CLK		50
#define HI3516CV300_UART1_CLK		51
#define HI3516CV300_UART2_CLK		52
#define HI3516CV300_SPI0_CLK		53
#define HI3516CV300_SPI1_CLK		54
#define HI3516CV300_FMC_CLK		55
#define HI3516CV300_MMC0_CLK		56
#define HI3516CV300_MMC1_CLK		57
#define HI3516CV300_MMC2_CLK		58
#define HI3516CV300_MMC3_CLK		59
#define HI3516CV300_ETH_CLK		60
#define HI3516CV300_USB2_BUS_CLK	62
#define HI3516CV300_UTMI0_CLK		63
#define HI3516CV300_DMAC_CLK		65
#define HI3516CV300_SENSOR_CLK		66
#define HI3516CV300_MIPI_CLK		67
#define HI3516CV300_ISP_CLK		68
#define HI3516CV300_VIU_CLK		69
#define HI3516CV300_VEDU_CLK		70
#define HI3516CV300_VPSS_CLK		71
#define HI3516CV300_VGS_CLK		72
#define HI3516CV300_JPGE_CLK		73
#define HI3516CV300_IVE_CLK		74
#define HI3516CV300_AIAO_CLK		75
#define HI3516CV300_PWM_CLK		76
#define HI3516CV300_IR_CLK		77
#define HI3516CV300_CIPHER_CLK		78
#define HI3516CV300_HASH_CLK		79
#define HI3516CV300_TRNG_CLK		80
#define HI3516CV300_RSA_CLK		81
#define HI3516CV300_KLAD_CLK		82
#define HI3516CV300_USB2_OHCI48M_CLK	83
#define HI3516CV300_USB2_OHCI12M_CLK	84
#define HI3516CV300_USB2_OTG_UTMI_CLK	85
#define HI3516CV300_USB2_HST_PHY_CLK	86
#define HI3516CV300_USB2_PHY_CLK	87

/* PLLs, and the CPU and DDR clock selects */
#define HI3516CV300_APLL_CLK		88
#define HI3516CV300_VPLL_CLK		89
#define HI3516CV300_EPLL_CLK		90
#define HI3516CV300_APLL_VCO2_CLK	91
#define HI3516CV300_CPU_MUX		92
#define HI3516CV300_DDR_DIV		93
#define HI3516CV300_DDR_MUX		94

#define HI3516CV300_CRG_NR_CLKS		128

/* sys controller clocks (timers) */
#define HI3516CV300_TIME00_CLK		1
#define HI3516CV300_TIME01_CLK		2
#define HI3516CV300_TIME10_CLK		3
#define HI3516CV300_TIME11_CLK		4

#define HI3516CV300_SYS_NR_CLKS		8
#endif /* __DTS_HI3516CV300_CLOCK_H */
