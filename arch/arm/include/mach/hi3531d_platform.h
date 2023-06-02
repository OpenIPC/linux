#ifndef __HI3531D_CHIP_REGS_H__
#define __HI3531D_CHIP_REGS_H__

/* -------------------------------------------------------------------- */
/* SRAM Base Address Register */
#define SRAM_BASE_ADDRESS       0x04010000

/* -------------------------------------------------------------------- */
#define FMC_REG_BASE            0x10000000
#define NFC_REG_BASE            0x10010000

/* -------------------------------------------------------------------- */
/* CORTTX-A9 internal Register */
/* -------------------------------------------------------------------- */
#define A9_PERI_BASE            0x10300000

#define REG_A9_PERI_SCU         0x0000
#define REG_A9_PERI_GIC_CPU     0x0100
#define REG_A9_PERI_GLOBAL_TIMER    0x0200
#define REG_A9_PERI_PRI_TIMER_WDT   0x0600
#define REG_A9_PERI_GIC_DIST        0x1000

#define CFG_GIC_CPU_BASE        (IO_ADDRESS(A9_PERI_BASE) \
		+ REG_A9_PERI_GIC_CPU)
#define CFG_GIC_DIST_BASE       (IO_ADDRESS(A9_PERI_BASE) \
		+ REG_A9_PERI_GIC_DIST)

 /* -------------------------------------------------------------------- */
#define GSF_REG_BASE            0x100a0000

/* -------------------------------------------------------------------- */
#define REG_BASE_L2CACHE        0x10700000

/* -------------------------------------------------------------------- */
#define TIMER0_REG_BASE         0x12000000
#define TIMER1_REG_BASE         0x12000020
#define TIMER2_REG_BASE         0x12010000
#define TIMER3_REG_BASE         0x12010020
#define TIMER4_REG_BASE         0x12020000
#define TIMER5_REG_BASE         0x12020020
#define TIMER6_REG_BASE         0x12030000
#define TIMER7_REG_BASE         0x12030020

/* -------------------------------------------------------------------- */
/* Clock and Reset Generator REG */
/* -------------------------------------------------------------------- */
#define CRG_REG_BASE            0x12040000
#define REG_BASE_CRG            CRG_REG_BASE

#define REG_CRG20           0x0050
#define REG_CRG32           0x0080
#define REG_CRG72           0x0120
#define REG_CRG75           0x012c
#define REG_CRG76           0x0130
#define REG_CRG77           0x0134
#define REG_CRG79           0x013c
#define REG_CRG81           0x0144
#define REG_CRG82           0x0148
#define REG_CRG83           0x014c
#define REG_CRG85           0x0154
#define REG_CRG87           0x015c
#define REG_CRG91           0x016c

/* SOC CRG register offset */
#define REG_SOC_CRG         REG_CRG20
#define GET_SYS_BUS_CLK(_reg)       (((_reg) >> 2) & 0x3)
#define SYS_CLK_XTAL            0
#define SYS_CLK_324M            1
#define SYS_CLK_375M            2
#define GET_PERI_AXI_CLK(_reg)      ((_reg) & 0x3)
#define PERI_CLK_XTAL           0
#define PERI_CLK_200M           1
#define PERI_CLK_300M           2

/* A9 soft reset request register offset */
#define REG_A9_SRST_CRG         REG_CRG32
#define WDG1_SRST_REQ           (0x1 << 6)
#define DBG1_SRST_REQ           (0x1 << 5)
#define CPU1_SRST_REQ           (0x1 << 4)

/* USB 3.0 CRG PHY register offset */
#define REG_USB3_PHY0           REG_CRG72

/* USB 3.0 CRG Control register offset */
#define REG_USB3_CTRL           REG_CRG75

/* USB 2.0 CRG Control register offset */
#define REG_USB2_CTRL           REG_CRG76

/* USB 2.0 CRG PHY register offset */
#define REG_USB2_PHY0           REG_CRG77
#define REG_USB2_PHY1           REG_CRG91

/* NFC CRG register offset */
#define REG_NFC_CRG         REG_CRG79
#define NFC_CLK_SEL(_clk)       (((_clk) & 0x3) << 2)
#define NFC_CLK_24M         0
#define NFC_CLK_200M            1
#define NFC_CLK_ENABLE          (1 << 1)

/* DMAC CRG register offset */
#define REG_DMAC_CRG            REG_CRG81
#define DMAC_CLK_EN         (0x1 << 1)
#define DMAC_SRST_REQ           (0x1 << 0)

/* FMC CRG register offset */
#define REG_FMC_CRG         REG_CRG82
#define FMC_CLK_SEL(_clk)       (((_clk) & 0x3) << 2)
#define FMC_CLK_SEL_MASK        (0x3 << 2)
#define FMC_CLK_24M         0
#define FMC_CLK_83M         1
#define FMC_CLK_125M            2
#define FMC_CLK_150M            3
#define FMC_CLK_ENABLE          (0x1 << 1)

/* Ethernet CRG register offset */
#define REG_ETH_CRG         REG_CRG83
#define REG_ETH_MAC_IF          REG_CRG87

/* Uart CRG register offset */
#define REG_UART_CRG            REG_CRG85
#define UART_CLK_SEL(_clk)      (((_clk) & 0x3) << 19)
#define UART_CLK_SEL_MASK       (0x3 << 19)
#define UART_CLK_APB            0
#define UART_CLK_24M            1
#define UART_CLK_2M         2

/* SSP CRG register offset */
#define REG_SSP_CRG         REG_CRG85
#define SSP_CLK_ENABLE          (0x1 << 13)
#define SSP_SOFT_RESET_REQ      (0x1 << 5)

/* -------------------------------------------------------------------- */
/* System Control REG */
/* -------------------------------------------------------------------- */
#define SYS_CTRL_BASE           0x12050000

/* System Control register offset */
#define REG_SC_CTRL         0x0000
#define SC_CTRL_TIMER7_CLK_SEL(_clk)    (((_clk) & 0x1) << 31)
#define SC_CTRL_TIMER6_CLK_SEL(_clk)    (((_clk) & 0x1) << 29)
#define SC_CTRL_TIMER5_CLK_SEL(_clk)    (((_clk) & 0x1) << 27)
#define SC_CTRL_TIMER4_CLK_SEL(_clk)    (((_clk) & 0x1) << 25)
#define SC_CTRL_TIMER3_CLK_SEL(_clk)    (((_clk) & 0x1) << 22)
#define SC_CTRL_TIMER2_CLK_SEL(_clk)    (((_clk) & 0x1) << 20)
#define SC_CTRL_TIMER1_CLK_SEL(_clk)    (((_clk) & 0x1) << 18)
#define SC_CTRL_TIMER0_CLK_SEL(_clk)    (((_clk) & 0x1) << 16)
#define TIMER_CLK_3M            0
#define TIMER_CLK_BUS           1

/* System soft reset register offset */
#define REG_SC_SYSRES           0x0004

#define REG_SC_SOFT_INT         0x001c
#define REG_SC_SOFT_TYPE        0x0020
#define REG_SC_LOCK_EN          0x0044

/* System Status register offset */
#define REG_SC_STAT         0x008c
#define SYS_CTRL_SYSSTAT    REG_SC_STAT

/* bit[8]=0; SPI nor flash; bit[8]=1: SPI nand flash */
#define GET_SPI_DEVICE_TYPE(_reg)   (((_reg) >> 8) & 0x1)
/* if bit[8]=0 SPI nor flash
 *  * bit[7]=0: 3-Byte address mode; bit[7]=1: 4-Byte address mode */
#define GET_SPI_NOR_ADDR_MODE(_reg) (((_reg) >> 7) & 0x1)

#define REG_SC_SYSID0           0x0EE0
#define REG_SC_SYSID1           0x0EE4
#define REG_SC_SYSID2           0x0EE8
#define REG_SC_SYSID3           0x0EEC

/* -------------------------------------------------------------------- */
/* UART Control REG */
/* -------------------------------------------------------------------- */
#define UART0_REG_BASE          0x12080000
#define UART1_REG_BASE          0x12090000
#define UART2_REG_BASE          0x120A0000
#define UART3_REG_BASE          0x12130000

#define REG_UART_DATA           0x0000
#define REG_UART_FLAG           0x0018
#define REG_UART_CTRL           0x0030
#define REG_UART_DMA_CR         0x0048

/* -------------------------------------------------------------------- */
/* I2C Control REG */
/* -------------------------------------------------------------------- */
#define I2C0_REG_BASE           0x120c0000
#define I2C1_REG_BASE           0x122e0000

#define REG_I2C_DATA            0x0010

/* -------------------------------------------------------------------- */
/* SSP Control REG */
/* -------------------------------------------------------------------- */
#define SSP_REG_BASE            0x120d0000

#define REG_SSP_DATA            0x0008

/* -------------------------------------------------------------------- */
/* Peripheral Control REG */
/* -------------------------------------------------------------------- */
#define MISC_REG_BASE           0x12120000

#define MISC_CTRL5          0x0014
#define MISC_CTRL36         0x0090
#define MISC_CTRL37         0x0094
#define MISC_CTRL74         0x0128
#define MISC_CTRL75         0x012c
#define MISC_CTRL78         0x0138

/* SPI Chip Select register offset */
#define REG_SSP_CS          MISC_CTRL5
#define SSP_CS_SEL(_cs)         (((_cs) & 0x3) << 0)
#define SSP_CS_SEL_MASK         (0x3 << 0)

/* USB 2.0 MISC Control register offset */
#define REG_USB2_CTRL0          MISC_CTRL36
#define REG_USB2_CTRL1          MISC_CTRL37

/* USB 3.0 MISC Control register offset */
#define REG_USB3_CTRL0          MISC_CTRL74
#define REG_USB3_CTRL1          MISC_CTRL75

#define REG_COMB_PHY1           MISC_CTRL78

/* hisilicon satav200 register */
#define HI_SATA_PORT_FIFOTH	0x44
#define HI_SATA_PORT_PHYCTL1	0x48
#define HI_SATA_PORT_PHYCTL	0x74

#define HI_SATA_PHY_CTL0	0xA0
#define HI_SATA_PHY_CTL1	0xA4
#define HI_SATA_PHY_CTL2	0xB0
#define HI_SATA_PHY_RST_BACK_MASK    0xAC

#define HI_SATA_FIFOTH_VALUE	0xFEED9F24

#define HI_SATA_BIGENDINE       (1 << 3)

#define HI_SATA_PHY_MODE_1_5G	0
#define HI_SATA_PHY_MODE_3G	1
#define HI_SATA_PHY_MODE_6G	2

#define HI_SATA_PHY_1_5G	0x0e180000
#define HI_SATA_PHY_3G		0x0e390000
#define HI_SATA_PHY_6G		0x0e5a0000

#define HI_SATA_PHY_SG_1_5G	0x50438
#define HI_SATA_PHY_SG_3G	0x50438
#define HI_SATA_PHY_SG_6G	0x50438

#define	HI_SATA_MISC_CTRL			IO_ADDRESS(0x12120000)
#define HI_SATA_MISC_COMB_PHY0		(HI_SATA_MISC_CTRL + 0x134)
#define HI_SATA_MISC_COMB_PHY1		(HI_SATA_MISC_CTRL + 0x138)
#define HI_SATA_MISC_COMB_PHY2		(HI_SATA_MISC_CTRL + 0x140)
#define HI_SATA_MISC_COMB_PHY3		(HI_SATA_MISC_CTRL + 0x144)

/* -------------------------------------------------------------------- */
#define FMC_MEM_BASE            0x14000000
#define NFC_MEM_BASE            0x15000000
#define DDR_MEM_BASE            0x40000000

#define CFG_TIMER_PER		(4)	/* AXI:APB is 4:1 */
/* -------------------------------------------------------------------- */
#define get_bus_clk() ({ \
		unsigned int base, regval, busclk = 0; \
		base = IO_ADDRESS(CRG_REG_BASE + REG_SOC_CRG); \
		regval = readl((void *)base); \
		regval = GET_PERI_AXI_CLK(regval); \
		if (PERI_CLK_200M == regval) \
			busclk = 200000000; \
		else if (PERI_CLK_300M == regval) \
			busclk = 300000000; \
		busclk; \
	})

#endif /* End of __HI3531D_CHIP_REGS_H__ */
