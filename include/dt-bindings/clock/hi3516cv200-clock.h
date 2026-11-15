/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2016 HiSilicon Technologies Co., Ltd.
 *
 * Clock identifiers of the CRG and system controller of the Hi3516C V200
 * and Hi3518E V200 (and V201), ARM926EJ-S / ARMv5TE. Both SoCs have the same
 * CRG register description; see hisilicon,hi3518ev200-crg.yaml. The CRGs of
 * the other HiSilicon IP camera SoCs (Hi3516C V100, Hi3518E V100, Hi3516E V200,
 * V300) differ, and their identifiers live in their own headers.
 *
 * The file name and the HI3516CV200_ prefix are the ones the device trees and
 * the clock driver already use.
 */

#ifndef __DTS_HI3516CV200_CLOCK_H
#define __DTS_HI3516CV200_CLOCK_H

/* fixed rate clocks */
#define HI3516CV200_FIXED_3M		1
#define HI3516CV200_FIXED_6M		2
#define HI3516CV200_FIXED_24M		3
#define HI3516CV200_FIXED_25M		4
#define HI3516CV200_FIXED_27M		5
#define HI3516CV200_FIXED_37P125M	6
#define HI3516CV200_FIXED_49P5M		7
#define HI3516CV200_FIXED_50M		8
#define HI3516CV200_FIXED_54M		9
#define HI3516CV200_FIXED_74P25M	10
#define HI3516CV200_FIXED_99M		11
#define HI3516CV200_FIXED_125M		12
#define HI3516CV200_FIXED_148P5M	13
#define HI3516CV200_FIXED_198M		14
#define HI3516CV200_FIXED_200M		15
#define HI3516CV200_FIXED_250M		16
#define HI3516CV200_FIXED_297M		17
#define HI3516CV200_FIXED_300M		18
#define HI3516CV200_FIXED_396M		19
#define HI3516CV200_FIXED_540M		20
#define HI3516CV200_FIXED_594M		21
#define HI3516CV200_FIXED_600M		22
#define HI3516CV200_FIXED_650M		23
#define HI3516CV200_FIXED_750M		24
#define HI3516CV200_FIXED_1188M		25

/* mux clocks (parent of gates / dividers) */
#define HI3516CV200_FMC_MUX		30
#define HI3516CV200_UART_MUX		31
#define HI3516CV200_ETH_MUX		32
#define HI3516CV200_USB2_CTRL_UTMI0_REQ	33
#define HI3516CV200_USB2_HRST_REQ	34
#define HI3516CV200_MMC0_MUX		35
#define HI3516CV200_MMC1_MUX		36
#define HI3516CV200_ETH_CORE_MUX	37
#define HI3516CV200_I2C_MUX		38
#define HI3516CV200_PWM_MUX		39

/* divider clocks */
#define HI3516CV200_SYSAPB_CLK		40

/* PLL and CPU/bus clock selects */
#define HI3516CV200_APLL_CLK		41
#define HI3516CV200_VPLL_CLK		42
#define HI3516CV200_EPLL_CLK		43
#define HI3516CV200_CPU_MUX		44
#define HI3516CV200_BUS_MUX		45

/* gate clocks */
#define HI3516CV200_FMC_CLK		50
#define HI3516CV200_UART0_CLK		51
#define HI3516CV200_UART1_CLK		52
#define HI3516CV200_UART2_CLK		53
#define HI3516CV200_UART3_CLK		54
#define HI3516CV200_ETH_CLK		55
#define HI3516CV200_MMC0_CLK		56
#define HI3516CV200_MMC1_CLK		57
#define HI3516CV200_SPI0_CLK		58
#define HI3516CV200_SPI1_CLK		59
#define HI3516CV200_DMAC_CLK		60
#define HI3516CV200_USB2_CLK		61
#define HI3516CV200_PWM_CLK		62
#define HI3516CV200_IR_CLK		63

/* imaging clocks, identifiers 64 to 127 are reserved for them */
#define HI3516CV200_SENSOR_MUX		64
#define HI3516CV200_VI_MUX		65
#define HI3516CV200_SENSOR_CLK		66
#define HI3516CV200_VI_CLK		67
#define HI3516CV200_MIPI_CLK		68
#define HI3516CV200_TDE_CLK		69
#define HI3516CV200_VGS_CLK		70
#define HI3516CV200_JPGE_CLK		71
#define HI3516CV200_VEDU_CLK		72
#define HI3516CV200_VPSS_CLK		73
#define HI3516CV200_AIAO_CLK		74
#define HI3516CV200_CIPHER_CLK		75

#define HI3516CV200_CRG_NR_CLKS		128

/* sysctrl (timer mux) clocks — exported via the system-controller provider */
#define HI3516CV200_TIME0_0_CLK		1
#define HI3516CV200_TIME0_1_CLK		2
#define HI3516CV200_TIME1_2_CLK		3
#define HI3516CV200_TIME1_3_CLK		4

#define HI3516CV200_SC_NR_CLKS		8

#endif	/* __DTS_HI3516CV200_CLOCK_H */
