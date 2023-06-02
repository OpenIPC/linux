/*
 *
 * Copyright (C) 2015 Fullhan.com
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __ASM_ARCH_FH8833_H
#define __ASM_ARCH_FH8833_H

#include <linux/init.h>

#define SRAM_GRANULARITY		32
#define SRAM_SIZE			SZ_32K

#define SIMPLE_TIMER_BASE		2

#define RAM_BASE			(0x10000000)
#define DDR_BASE			(0xA0000000)

#define PMU_REG_BASE			(0xF0000000)
#define TIMER_REG_BASE			(0xF0C00000)
#define GPIO0_REG_BASE			(0xF0300000)
#define GPIO1_REG_BASE			(0xF4000000)
#define UART0_REG_BASE			(0xF0700000)
#define UART1_REG_BASE			(0xF0800000)
#define SPI0_REG_BASE			(0xF0500000)
#define SPI1_REG_BASE			(0xF0600000)
#define SPI2_REG_BASE			(0xF0640000)
#define INTC_REG_BASE			(0xE0200000)
#define GMAC_REG_BASE			(0xE0600000)
#define USBC_REG_BASE			(0xE0700000)
#define DMAC_REG_BASE			(0xE0300000)
#define I2C1_REG_BASE			(0xF0B00000)
#define I2C0_REG_BASE			(0xF0200000)
#define SDC0_REG_BASE			(0xE2000000)
#define SDC1_REG_BASE			(0xE2200000)
#define WDT_REG_BASE			(0xF0D00000)
#define PWM_REG_BASE			(0xF0400000)
#define PAE_REG_BASE			(0xE7000000)
#define I2S_REG_BASE			(0xF0900000)
#define ACW_REG_BASE			(0xF0A00000)
#define SADC_REG_BASE			(0xF1200000)
#define EFUSE_REG_BASE			(0xF1600000)
#define AES_REG_BASE			(0xE8200000)
#define RTC_REG_BASE			(0xF1500000)
#define DDRC_REG_BASE			(0xED000000)
#define CONSOLE_REG_BASE		UART0_REG_BASE
#define FH_UART_NUMBER			2

#define PMU_REG_SIZE                  0x2018
#define PMU_DEBUG

#define REG_PMU_CHIP_ID                  (0x0000)
#define REG_PMU_IP_VER                   (0x0004)
#define REG_PMU_FW_VER                   (0x0008)
#define REG_PMU_SYS_CTRL                 (0x000c)
#define REG_PMU_PLL0                     (0x0010)
#define REG_PMU_PLL1                     (0x0014)
#define REG_PMU_PLL2                     (0x0018)
#define REG_PMU_CLK_GATE                 (0x001c)
#define REG_PMU_CLK_SEL                  (0x0020)
#define REG_PMU_CLK_DIV0                 (0x0024)
#define REG_PMU_CLK_DIV1                 (0x0028)
#define REG_PMU_CLK_DIV2                 (0x002c)
#define REG_PMU_CLK_DIV3                 (0x0030)
#define REG_PMU_CLK_DIV4                 (0x0034)
#define REG_PMU_CLK_DIV5                 (0x0038)
#define REG_PMU_CLK_DIV6                 (0x003c)
#define REG_PMU_SWRST_MAIN_CTRL          (0x0040)
#define REG_PMU_SWRST_AXI_CTRL           (0x0044)
#define REG_PMU_SWRST_AHB_CTRL           (0x0048)
#define REG_PMU_SWRST_APB_CTRL           (0x004c)
#define REG_PMU_SPC_IO_STATUS            (0x0054)
#define REG_PMU_SPC_FUN                  (0x0058)
#define REG_PMU_DBG_SPOT0                (0x005c)
#define REG_PMU_DBG_SPOT1                (0x0060)
#define REG_PMU_DBG_SPOT2                (0x0064)
#define REG_PMU_DBG_SPOT3                (0x0068)

#define REG_PMU_PAD_CIS_HSYNC_CFG        (0x0080)
#define REG_PMU_PAD_CIS_VSYNC_CFG        (0x0084)
#define REG_PMU_PAD_CIS_PCLK_CFG         (0x0088)
#define REG_PMU_PAD_CIS_D_0_CFG          (0x008c)
#define REG_PMU_PAD_CIS_D_1_CFG          (0x0090)
#define REG_PMU_PAD_CIS_D_2_CFG          (0x0094)
#define REG_PMU_PAD_CIS_D_3_CFG          (0x0098)
#define REG_PMU_PAD_CIS_D_4_CFG          (0x009c)
#define REG_PMU_PAD_CIS_D_5_CFG          (0x00a0)
#define REG_PMU_PAD_CIS_D_6_CFG          (0x00a4)
#define REG_PMU_PAD_CIS_D_7_CFG          (0x00a8)
#define REG_PMU_PAD_CIS_D_8_CFG          (0x00ac)
#define REG_PMU_PAD_CIS_D_9_CFG          (0x00b0)
#define REG_PMU_PAD_CIS_D_10_CFG         (0x00b4)
#define REG_PMU_PAD_CIS_D_11_CFG         (0x00b8)
#define REG_PMU_PAD_MAC_RMII_CLK_CFG     (0x00bc)
#define REG_PMU_PAD_MAC_REF_CLK_CFG      (0x00c0)
#define REG_PMU_PAD_MAC_MDC_CFG          (0x00c4)
#define REG_PMU_PAD_MAC_MDIO_CFG         (0x00c8)
#define REG_PMU_PAD_MAC_COL_MII_CFG      (0x00cc)
#define REG_PMU_PAD_MAC_CRS_MII_CFG      (0x00d0)
#define REG_PMU_PAD_MAC_RXCK_CFG         (0x00d4)
#define REG_PMU_PAD_MAC_RXD0_CFG         (0x00d8)
#define REG_PMU_PAD_MAC_RXD1_CFG         (0x00dc)
#define REG_PMU_PAD_MAC_RXD2_MII_CFG     (0x00e0)
#define REG_PMU_PAD_MAC_RXD3_MII_CFG     (0x00e4)
#define REG_PMU_PAD_MAC_RXDV_CFG         (0x00e8)
#define REG_PMU_PAD_MAC_TXCK_CFG         (0x00ec)
#define REG_PMU_PAD_MAC_TXD0_CFG         (0x00f0)
#define REG_PMU_PAD_MAC_TXD1_CFG         (0x00f4)
#define REG_PMU_PAD_MAC_TXD2_MII_CFG     (0x00f8)
#define REG_PMU_PAD_MAC_TXD3_MII_CFG     (0x00fc)
#define REG_PMU_PAD_MAC_TXEN_CFG         (0x0100)
#define REG_PMU_PAD_MAC_RXER_MII_CFG     (0x0104)
#define REG_PMU_PAD_MAC_TXER_MII_CFG     (0x0108)
#define REG_PMU_PAD_GPIO_0_CFG           (0x010c)
#define REG_PMU_PAD_GPIO_1_CFG           (0x0110)
#define REG_PMU_PAD_GPIO_2_CFG           (0x0114)
#define REG_PMU_PAD_GPIO_3_CFG           (0x0118)
#define REG_PMU_PAD_GPIO_4_CFG           (0x011c)
#define REG_PMU_PAD_GPIO_5_CFG           (0x0120)
#define REG_PMU_PAD_GPIO_6_CFG           (0x0124)
#define REG_PMU_PAD_GPIO_7_CFG           (0x0128)
#define REG_PMU_PAD_GPIO_8_CFG           (0x012c)
#define REG_PMU_PAD_GPIO_9_CFG           (0x0130)
#define REG_PMU_PAD_GPIO_10_CFG          (0x0134)
#define REG_PMU_PAD_GPIO_11_CFG          (0x0138)
#define REG_PMU_PAD_GPIO_12_CFG          (0x013c)
#define REG_PMU_PAD_GPIO_13_CFG          (0x0140)
#define REG_PMU_PAD_GPIO_14_CFG          (0x0144)
#define REG_PMU_PAD_UART_RX_CFG          (0x0148)
#define REG_PMU_PAD_UART_TX_CFG          (0x014c)
#define REG_PMU_PAD_CIS_SCL_CFG          (0x0150)
#define REG_PMU_PAD_CIS_SDA_CFG          (0x0154)
#define REG_PMU_PAD_I2C_SCL_CFG          (0x0158)
#define REG_PMU_PAD_I2C_SDA_CFG          (0x015c)
#define REG_PMU_PAD_SSI0_CLK_CFG         (0x0160)
#define REG_PMU_PAD_SSI0_TXD_CFG         (0x0164)
#define REG_PMU_PAD_SSI0_CSN_0_CFG       (0x0168)
#define REG_PMU_PAD_SSI0_CSN_1_CFG       (0x016c)
#define REG_PMU_PAD_SSI0_RXD_CFG         (0x0170)
#define REG_PMU_PAD_SD0_CD_CFG           (0x0174)
#define REG_PMU_PAD_SD0_WP_CFG           (0x0178)
#define REG_PMU_PAD_SD0_CLK_CFG          (0x017c)
#define REG_PMU_PAD_SD0_CMD_RSP_CFG      (0x0180)
#define REG_PMU_PAD_SD0_DATA_0_CFG       (0x0184)
#define REG_PMU_PAD_SD0_DATA_1_CFG       (0x0188)
#define REG_PMU_PAD_SD0_DATA_2_CFG       (0x018c)
#define REG_PMU_PAD_SD0_DATA_3_CFG       (0x0190)
#define REG_PMU_PAD_SD1_CD_CFG           (0x0194)
#define REG_PMU_PAD_SD1_WP_CFG           (0x0198)
#define REG_PMU_PAD_SD1_CLK_CFG          (0x019c)
#define REG_PMU_PAD_SD1_CMD_RSP_CFG      (0x01a0)
#define REG_PMU_PAD_SD1_DATA_0_CFG       (0x01a4)
#define REG_PMU_PAD_SD1_DATA_1_CFG       (0x01a8)
#define REG_PMU_PAD_SD1_DATA_2_CFG       (0x01ac)
#define REG_PMU_PAD_SD1_DATA_3_CFG       (0x01b0)
#define REG_PMU_AXI0_PRIO_CFG0           (0x01b4)
#define REG_PMU_AXI0_PRIO_CFG1           (0x01b8)
#define REG_PMU_AXI1_PRIO_CFG0           (0x01bc)
#define REG_PMU_AXI1_PRIO_CFG1           (0x01c0)
#define REG_PMU_SWRSTN_NSR               (0x01c4)
#define REG_PMU_ARM_INT_0                (0x01e0)
#define REG_PMU_ARM_INT_1                (0x01e4)
#define REG_PMU_ARM_INT_2                (0x01e8)
#define REG_PMU_A625_INT_0               (0x01ec)
#define REG_PMU_A625_INT_1               (0x01f0)
#define REG_PMU_A625_INT_2               (0x01f4)
#define REG_PMU_DMA                      (0x01f8)
#define REG_PMU_WDT_CTRL                 (0x01fc)
#define REG_PMU_DBG_STAT0                (0x0200)
#define REG_PMU_DBG_STAT1                (0x0204)
#define REG_PMU_DBG_STAT2                (0x0208)
#define REG_PMU_DBG_STAT3                (0x020c)
#define REG_PMU_USB_SYS                  (0x0210)
#define REG_PMU_USB_CFG                  (0x0214)
#define REG_PMU_USB_TUNE                 (0x0218)
#define REG_PMU_PAD_CIS_CLK_CFG          (0x021c)
#define REG_PMU_PAEARCBOOT0              (0x1000)
#define REG_PMU_PAEARCBOOT1              (0x1004)
#define REG_PMU_PAEARCBOOT2              (0x1008)
#define REG_PMU_PAEARCBOOT3              (0x100c)
#define REG_PMU_PAE_ARC_START_CTRL       (0x1010)
#define REG_PMU_A625BOOT0                (0x2000)
#define REG_PMU_A625BOOT1                (0x2004)
#define REG_PMU_A625BOOT2                (0x2008)
#define REG_PMU_A625BOOT3                (0x200c)
#define REG_PMU_A625_START_CTRL          (0x2010)
#define REG_PMU_ARC_INTC_MASK            (0x2014)
#define REG_PMU_PAE_ARC_INTC_MASK        (0x2018)

/*ATTENTION: written by ARC */
#define PMU_ARM_INT_MASK             (0x01e0)
#define PMU_ARM_INT_RAWSTAT          (0x01e4)
#define PMU_ARM_INT_STAT             (0x01e8)

#define PMU_A625_INT_MASK             (0x01ec)
#define PMU_A625_INT_RAWSTAT          (0x01f0)
#define PMU_A625_INT_STAT             (0x01f4)

#define ARM_PMU_IRQ		0
#define DDRC_IRQ		1
#define WDT_IRQ			2
#define TMR0_IRQ		3
#define PAE_ARC_IRQ0		4
#define PAE_ARC_IRQ1		5
#define PAE_ARC_IRQ2		6
#define ISPP_IRQ		7
#define ISPF_IRQ		8
#define VPU_IRQ			9
#define PAE_IRQ			10
#define I2C0_IRQ		11
#define I2C1_IRQ		12
#define JPEG_IRQ		13
#define BGM_IRQ			14
#define GMAC_IRQ		15
#define AES_IRQ			16
#define SDC0_IRQ		17
#define SDC1_IRQ		18
#define ACW_IRQ			19
#define SADC_IRQ		20
#define SPI1_IRQ		21
#define SPI2_IRQ		22
#define DMAC0_IRQ		23
#define DMAC1_IRQ		24
#define I2S0_IRQ		25
#define GPIO0_IRQ		26
#define USBC_IRQ		27
#define SPI0_IRQ		28
#define ARC_SW_IRQ		29
#define UART0_IRQ		30
#define UART1_IRQ		31
#define ARM_SW_IRQ		32
#define RTC_IRQ			33
#define AHBC0_IRQ		34
#define AHBC1_IRQ		35
#define PWM_IRQ			36
#define MIPIC_IRQ		37
#define MIPI_WRAP_IRQ		38

#define GPIO1_IRQ		40
#define USBC_IDHV_IRQ		41
#define USBC_OTG_IRQ		42
#define USBC_DP_IRQ		43
#define USBC_DM_IRQ		44

#define NR_INTERNAL_IRQS	(64)
#define NR_EXTERNAL_IRQS	(64)
#define NR_IRQS			(NR_INTERNAL_IRQS + NR_EXTERNAL_IRQS)

#endif /* __ASM_ARCH_FH8833_H */
