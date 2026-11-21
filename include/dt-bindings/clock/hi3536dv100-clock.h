/* SPDX-License-Identifier: GPL-2.0-only OR BSD-2-Clause */
/*
 * Clock identifiers of the CRG and the system controller of the Hi3536D V100
 * (a single Cortex-A7 NVR decoder SoC). The Hi3536E has the same SCSYSID and
 * uses these identifiers too. The CRGs of the other HiSilicon SoCs differ,
 * and their identifiers live in their own headers. The identifiers are per
 * provider: the CRG and the system controller both count from 1, so a CRG
 * identifier must not be used with the system controller or the reverse.
 *
 * The CRG reset specifiers are two cells, the register offset and the bit
 * number (see hisilicon,hi3536dv100-crg.yaml). The bits that exist are:
 *
 *   0x88  0 VDP
 *   0x8c  6 HDMI TX bus, 7 HDMI TX control, 9 HDMI SSC divider
 *   0x90  4 HDMI TX PHY, 5 HDMI TMDS divider
 *   0x94  0 TDE
 *   0x98  0 VGS
 *   0x9c  0 JPGE
 *   0xa0  0 JPGD
 *   0xa4  4 VDH MFD, 5 VDH SCD, 6 VDH all
 *   0xa8  0 CIPHER
 *   0xac  0 AIAO, 10 CODEC
 *   0xb0  1 SATA PHY lane
 *   0xb4  8 SATA bus, 9 SATA CKO_ALIVE, 10 SATA RX0, 11 SATA port 0
 *   0xb8  12 USB 2.0 bus, 13 UTMI0, 14 UTMI1, 16 HST_PHY
 *   0xbc  1 USB 2.0 PHY, 2 PHY port 0, 3 PHY port 1, 4 PHY test
 *   0xc0  0 FMC
 *   0xc4  0 Ethernet MAC, 3 external FE PHY, 9 internal FE PHY
 *   0xc8  4 DMAC
 *   0xcc  1 I2C, 3 RTC, 4 IR, 7 UART0, 8 UART1, 9 UART2
 *
 * A set bit holds the block in reset. Several of them are set at reset (the
 * VDH, SATA PHY lane, USB 2.0 and FE PHY resets), so a consumer has to
 * deassert them. The DDR resets of 0x84 and the DDRTEST reset (0xc8 bit 2) are
 * not listed: they must never be touched by a driver, and the reset controller
 * rejects them, like every register or bit that is not listed.
 */

#ifndef __DT_BINDINGS_CLOCK_HI3536DV100_H
#define __DT_BINDINGS_CLOCK_HI3536DV100_H

/* fixed rate clocks */
#define HI3536DV100_FIXED_2M		1
#define HI3536DV100_FIXED_3M		2
#define HI3536DV100_FIXED_24M		3
#define HI3536DV100_FIXED_25M		4
#define HI3536DV100_FIXED_27M		5
#define HI3536DV100_FIXED_50M		6
#define HI3536DV100_FIXED_54M		7
#define HI3536DV100_FIXED_83P3M		8
#define HI3536DV100_FIXED_100M		9
#define HI3536DV100_FIXED_150M		10
#define HI3536DV100_FIXED_202P5M	11
#define HI3536DV100_FIXED_500M		12

/*
 * Clock inputs of the Ethernet interface: the MII_RXCK and MII_TXCK pads
 * (the rate follows the link speed, so the clock reports 0 Hz) and the
 * RMII_CLK pad (50 MHz for RMII).
 */
#define HI3536DV100_MII_PAD_CLK		13
#define HI3536DV100_RMII_PAD_CLK	14

/* PLLs (read only) */
#define HI3536DV100_APLL_CLK		20
#define HI3536DV100_VPLL0_CLK		21
#define HI3536DV100_VPLL1_CLK		22
#define HI3536DV100_VPLL2_CLK		23
#define HI3536DV100_EPLL_CLK		24
#define HI3536DV100_DPLL_CLK		25

/*
 * Core clock selects of PERI_CRG20 (read only: a switch needs the PLL lock
 * and the PERI_CRG21 handshake). The system APB clock is the clock of the
 * UARTs, timers, GPIOs and the watchdog bus interface.
 */
#define HI3536DV100_A7_MUX		30
#define HI3536DV100_DDR_MUX		31
#define HI3536DV100_SYSBUS_MUX		32
#define HI3536DV100_MDAAPB_MUX		33
#define HI3536DV100_SYSAPB_CLK		34

/* module clock selects (parent of gates) */
#define HI3536DV100_UART_MUX		40
#define HI3536DV100_FMC_MUX		41
#define HI3536DV100_ETH_CORE_MUX	42
#define HI3536DV100_ETH_PHY_MUX		43
#define HI3536DV100_ETH_RMII_MUX	44
#define HI3536DV100_ETH_IF_MUX		45
#define HI3536DV100_SATA_REF_MUX	46
#define HI3536DV100_VDP_MUX		47
#define HI3536DV100_AIAO_CORE_MUX	48
#define HI3536DV100_AIAO_MCLK_MUX	49

/* gate clocks: peripherals */
#define HI3536DV100_UART0_CLK		60
#define HI3536DV100_UART1_CLK		61
#define HI3536DV100_UART2_CLK		62
#define HI3536DV100_IR_CLK		63
#define HI3536DV100_FMC_CLK		64
#define HI3536DV100_ETH_CLK		65
#define HI3536DV100_ETH_FEPHY_CLK	66
#define HI3536DV100_DMAC_CLK		67
#define HI3536DV100_CIPHER_CLK		68

/* gate clocks: SATA and USB 2.0 */
#define HI3536DV100_SATA_PHY_REF_CLK	70
#define HI3536DV100_SATA_BUS_CLK	71
#define HI3536DV100_SATA_RX0_CLK	72
#define HI3536DV100_SATA_TX0_CLK	73
#define HI3536DV100_SATA_CKO_ALIVE_CLK	74
#define HI3536DV100_USB2_BUS_CLK	80
#define HI3536DV100_USB2_OHCI48M_CLK	81
#define HI3536DV100_USB2_OHCI12M_CLK	82
#define HI3536DV100_USB2_HST_PHY_CLK	83
#define HI3536DV100_USB2_UTMI0_CLK	84
#define HI3536DV100_USB2_UTMI1_CLK	85
#define HI3536DV100_USB2_PHY_CLK	86

/* gate clocks: media blocks */
#define HI3536DV100_TDE_CLK		90
#define HI3536DV100_VGS_CLK		91
#define HI3536DV100_JPGE_CLK		92
#define HI3536DV100_JPGD_CLK		93
#define HI3536DV100_VDH_CLK		94
#define HI3536DV100_AIAO_CLK		95
#define HI3536DV100_CODEC_CLK		96
#define HI3536DV100_VOU_CLK		100
#define HI3536DV100_VOU_HD0_CLK		101
#define HI3536DV100_VOU_BUS_CLK		102
#define HI3536DV100_HD_DAC_CLK		103
#define HI3536DV100_HDMI_PXL_CLK	104
#define HI3536DV100_HDMI_SSC_IN_CLK	105
#define HI3536DV100_HDMI_SSC_BYPASS_CLK	106
#define HI3536DV100_HDMI_OSC24M_CLK	107
#define HI3536DV100_HDMI_TMDS_CLK	108

#define HI3536DV100_CRG_NR_CLKS		112

/*
 * sysctrl clocks: the count clock selects of the eight SP804 timers (SC_CTRL
 * bits 16, 18, 20, 22, 25, 27, 29 and 31), named after the timer pair that
 * contains them.
 */
#define HI3536DV100_TIME0_0_CLK		1
#define HI3536DV100_TIME0_1_CLK		2
#define HI3536DV100_TIME1_2_CLK		3
#define HI3536DV100_TIME1_3_CLK		4
#define HI3536DV100_TIME2_4_CLK		5
#define HI3536DV100_TIME2_5_CLK		6
#define HI3536DV100_TIME3_6_CLK		7
#define HI3536DV100_TIME3_7_CLK		8

#define HI3536DV100_SC_NR_CLKS		16

#endif	/* __DT_BINDINGS_CLOCK_HI3536DV100_H */
