#ifndef __ASM_ARCH_HL_H
#define __ASM_ARCH_HL_H

#include <linux/init.h>

#define SRAM_GRANULARITY		32
#define SRAM_SIZE			(SZ_128K+SZ_8K)


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
#define I2C2_REG_BASE                   (0xF0100000)
#define SDC0_REG_BASE			(0xE2000000)
#define SDC1_REG_BASE			(0xE2200000)
#define WDT_REG_BASE			(0xF0D00000)
#define PWM_REG_BASE			(0xF0400000)
#define I2S_REG_BASE			(0xF0900000)
#define ACW_REG_BASE			(0xF0A00000)
#define UART2_REG_BASE			(0xF1300000)
#define SADC_REG_BASE			(0xF1200000)
#define EFUSE_REG_BASE			(0xF1600000)
#define AES_REG_BASE			(0xE8200000)
#define RTC_REG_BASE			(0xF1500000)
#define DDRC_REG_BASE			(0xED000000)
#define CONSOLE_REG_BASE		UART0_REG_BASE
#define FH_UART_NUMBER			3

#define FH_PMU_REG_SIZE	0x2110
#define REG_PMU_CHIP_ID                  (0x0000)
#define REG_PMU_IP_VER                   (0x0004)
#define REG_PMU_FW_VER                   (0x0008)
#define REG_PMU_CLK_SEL                  (0x000c)
/*for HL REG_PMU_SYS_CTRL and CLK_SEL use one register */
#define REG_PMU_SYS_CTRL                 (0x000c)
#define REG_PMU_PLL0                     (0x0010)
#define REG_PMU_PLL1                     (0x0014)
#define REG_PMU_PLL0_CTRL               (0x0018)
#define REG_PMU_CLK_GATE                 (0x001c)
#define REG_PMU_CLK_GATE1                (0x0020)
#define REG_PMU_CLK_DIV0                 (0x0024)
#define REG_PMU_CLK_DIV1                 (0x0028)
#define REG_PMU_CLK_DIV2                 (0x002c)
#define REG_PMU_CLK_DIV3                 (0x0030)
#define REG_PMU_CLK_DIV4                 (0x0034)
#define REG_PMU_CLK_DIV5                 (0x0038)
#define REG_PMU_CLK_DIV6                 (0x003c)
#define REG_PMU_SWRST_MAIN_CTRL          (0x0040)
#define REG_PMU_SWRST_MAIN_CTRL2         (0x0044)
#define REG_PMU_SWRST_AHB_CTRL           (0x0048)
#define REG_PMU_SWRST_APB_CTRL           (0x004c)
#define REG_PMU_SPC_IO_STATUS            (0x0054)
#define REG_PMU_SPC_FUN                  (0x0058)
#define REG_PMU_CLK_DIV7                 (0x005c)
#define REG_PMU_CLK_DIV8                 (0x0060)
#define REG_PMU_PLL2                     (0x0064)
#define REG_PMU_PLL2_CTRL                (0x0068)
#define REG_PMU_PLL1_CTRL                (0x006c)
#define REG_PAD_PWR_SEL                  (0x0074)
#define REG_PMU_SWRSTN_NSR               (0x0078)
#define REG_PMU_SWRSTN_NSR1              (0x007c)
#define REG_PMU_ETHPHY_REG0              (0x2108)


#define REG_PMU_PAD_BOOT_MODE_CFG        (0x0080)
#define REG_PMU_PAD_BOOT_SEL1_CFG        (0x0084)
#define REG_PMU_PAD_BOOT_SEL0_CFG        (0x0088)
#define REG_PMU_PAD_UART0_TX_CFG         (0x008c)
#define REG_PMU_PAD_UART0_RX_CFG         (0x0090)
#define REG_PMU_PAD_I2C0_SCL_CFG         (0x0094)
#define REG_PMU_PAD_I2C0_SDA_CFG         (0x0098)
#define REG_PMU_PAD_SENSOR_CLK_CFG       (0x009c)
#define REG_PMU_PAD_SENSOR_RSTN_CFG      (0x00a0)
#define REG_PMU_PAD_UART1_TX_CFG         (0x00a4)
#define REG_PMU_PAD_UART1_RX_CFG         (0x00a8)
#define REG_PMU_PAD_I2C1_SCL_CFG         (0x00ac)
#define REG_PMU_PAD_I2C1_SDA_CFG         (0x00b0)
#define REG_PMU_PAD_UART2_TX_CFG         (0x00b4)
#define REG_PMU_PAD_UART2_RX_CFG         (0x00b8)
#define REG_PMU_PAD_USB_PWREN_CFG        (0x00bc)
#define REG_PMU_PAD_PWM0_CFG             (0x00c0)
#define REG_PMU_PAD_PWM1_CFG             (0x00c4)
#define REG_PMU_PAD_PWM2_CFG             (0x00c8)
#define REG_PMU_PAD_PWM3_CFG             (0x00cc)
#define REG_PMU_PAD_MAC_RMII_CLK_CFG     (0x00d0)
#define REG_PMU_PAD_MAC_REF_CLK_CFG      (0x00d4)
#define REG_PMU_PAD_MAC_TXD0_CFG         (0x00d8)
#define REG_PMU_PAD_MAC_TXD1_CFG         (0x00dc)
#define REG_PMU_PAD_MAC_TXEN_CFG         (0x00e0)
#define REG_PMU_PAD_MAC_RXD0_CFG         (0x00e4)
#define REG_PMU_PAD_MAC_RXD1_CFG         (0x00e8)
#define REG_PMU_PAD_MAC_RXDV_CFG         (0x00ec)
#define REG_PMU_PAD_MAC_MDC_CFG          (0x00f0)
#define REG_PMU_PAD_MAC_MDIO_CFG         (0x00f4)
#define REG_PMU_PAD_SD1_CLK_CFG          (0x00f8)
#define REG_PMU_PAD_SD1_CD_CFG           (0x00fc)
#define REG_PMU_PAD_SD1_CMD_RSP_CFG      (0x0100)
#define REG_PMU_PAD_SD1_DATA_0_CFG       (0x0104)
#define REG_PMU_PAD_SD1_DATA_1_CFG       (0x0108)
#define REG_PMU_PAD_SD1_DATA_2_CFG       (0x010c)
#define REG_PMU_PAD_SD1_DATA_3_CFG       (0x0110)
#define REG_PMU_PAD_GPIO_0_CFG           (0x0114)
#define REG_PMU_PAD_GPIO_1_CFG           (0x0118)
#define REG_PMU_PAD_GPIO_2_CFG           (0x011c)
#define REG_PMU_PAD_GPIO_3_CFG           (0x0120)
#define REG_PMU_PAD_GPIO_4_CFG           (0x0124)
#define REG_PMU_PAD_SSI0_CLK_CFG         (0x0128)
#define REG_PMU_PAD_SSI0_CSN_0_CFG       (0x012c)
#define REG_PMU_PAD_SSI0_TXD_CFG         (0x0130)
#define REG_PMU_PAD_SSI0_RXD_CFG         (0x0134)
#define REG_PMU_PAD_SSI0_D2_CFG          (0x0138)
#define REG_PMU_PAD_SSI0_D3_CFG          (0x013c)
#define REG_PMU_PAD_SSI1_CLK_CFG         (0x0140)
#define REG_PMU_PAD_SSI1_CSN_0_CFG       (0x0144)
#define REG_PMU_PAD_SSI1_TXD_CFG         (0x0148)
#define REG_PMU_PAD_SSI1_RXD_CFG         (0x014c)
#define REG_PMU_PAD_SD0_CD_CFG           (0x0150)
#define REG_PMU_PAD_SD0_CLK_CFG          (0x0154)
#define REG_PMU_PAD_SD0_CMD_RSP_CFG      (0x0158)
#define REG_PMU_PAD_SD0_DATA_0_CFG       (0x015c)
#define REG_PMU_PAD_SD0_DATA_1_CFG       (0x0160)
#define REG_PMU_PAD_SD0_DATA_2_CFG       (0x0164)
#define REG_PMU_PAD_SD0_DATA_3_CFG       (0x0168)
#define REG_PMU_PAD_SADC_XAIN0_CFG       (0x016c)
#define REG_PMU_PAD_SADC_XAIN1_CFG       (0x0170)
#define REG_PMU_PAD_SADC_XAIN2_CFG       (0x0174)
#define REG_PMU_PAD_SADC_XAIN3_CFG       (0x0178)
#define REG_PMU_PAD_GPIO_28_CFG          (0x017c)
#define REG_PMU_PAD_GPIO_29_CFG          (0x0180)

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
#define REG_PMU_USB_SYS1                 (0x0228)
#define REG_PMU_PTSLO                    (0x022c)
#define REG_PMU_PTSHI                    (0x0230)
#define REG_PMU_USER0                    (0x0234)
#define REG_PMU_BOOT_MODE                (0x0330)
#define REG_PMU_DDR_SIZE                 (0x0334)
#define REG_PMU_CHIP_INFO                (0x033C)
#define REG_PMU_EPHY_PARAM               (0x0340)
#define REG_PMU_RTC_PARAM                (0x0344)
#define REG_PMU_SD1_FUNC_SEL             (0x03a0)
#define REG_PMU_PRDCID_CTRL0			 (0x0500)
#define REG_PMU_A625BOOT0                (0x2000)
#define REG_PMU_A625BOOT1                (0x2004)
#define REG_PMU_A625BOOT2                (0x2008)
#define REG_PMU_A625BOOT3                (0x200c)
#define REG_PMU_A625_START_CTRL          (0x2010)
#define REG_PMU_ARC_INTC_MASK            (0x2014)

#define FH_GMAC_AHB_RESET					(1<<17)
#define FH_GMAC_SPEED_100M					(1<<24)
#define PMU_RMII_SPEED_MODE (REG_PMU_CLK_SEL)
#define PMU_RXDV_GPIO_SWITCH (REG_PMU_PAD_MAC_RXDV_CFG)
#define PMU_RXDV_GPIO_MASK (0x0f000000)
#define PMU_RXDV_GPIO_VAL (0x01000000)

#define PMU_DWI2S_CLK_SEL_REG   (REG_PMU_CLK_SEL)
#define PMU_DWI2S_CLK_SEL_SHIFT (1)
#define PMU_DWI2S_CLK_DIV_REG   (REG_PMU_CLK_DIV6)
#define PMU_DWI2S_CLK_DIV_SHIFT (0)

/*ATTENTION: written by ARC */
#define PMU_ARM_INT_MASK             (0x01ec)
#define PMU_ARM_INT_RAWSTAT          (0x01f0)
#define PMU_ARM_INT_STAT             (0x01f4)

#define PMU_A625_INT_MASK             (0x01e0)
#define PMU_A625_INT_RAWSTAT          (0x01e4)
#define PMU_A625_INT_STAT             (0x01e8)

#define PMU_IRQ			0
#define DDRC_IRQ		1
#define WDT_IRQ			2
#define TMR0_IRQ		3
#define VEU_IRQ			4
#define PERF_IRQ	    5
#define VPU_IRQ			9
#define I2C0_IRQ		11
#define I2C1_IRQ		12
#define JPEG_IRQ		13
#define BGM_IRQ			14
#define VEU_LOOP_IRQ    15
#define AES_IRQ			16
#define MIPIC_IRQ		17
#define MIPI_WRAP_IRQ   18
#define ACW_IRQ			19
#define SADC_IRQ		20
#define SPI1_IRQ		21
#define JPEG_LOOP_IRQ	22
#define DMAC0_IRQ		23
#define DMAC1_IRQ		24
#define I2S0_IRQ		25
#define GPIO0_IRQ		26
#define SPI0_IRQ		28
#define ARC_SW_IRQ		29
#define UART0_IRQ		30
#define UART1_IRQ		31
#define ARM_SW_IRQ		32
#define RTC_IRQ			33
#define PWM_IRQ			36
#define SPI2_IRQ		38
#define USBC_IRQ		39
#define GPIO1_IRQ		40
#define UART2_IRQ		41
#define SDC0_IRQ		42
#define SDC1_IRQ		43
#define GMAC_IRQ		44
#define EPHY_IRQ		45
#define I2C2_IRQ		46
#define RTC_ALM_IRQ     47
#define RTC_CORE_IRQ    48
/* because chips with some same function in different */
/* pmu register, use wrap marco to make code to be same */
#define PMU_RMII_SPEED_MODE (REG_PMU_CLK_SEL)

#define MEM_START_PHY_ADDR	DDR_BASE
#define MEM_SIZE			0x4000000


#define NR_INTERNAL_IRQS	(64)
#define NR_EXTERNAL_IRQS	(64)
/*#define NR_IRQS			(NR_INTERNAL_IRQS + NR_EXTERNAL_IRQS)*/

/* SWRST_MAIN_CTRL */
#define CPU_RSTN_BIT			(0)
#define UTMI_RSTN_BIT			(1)
#define DDRPHY_RSTN_BIT			(2)
#define DDRC_RSTN_BIT			(3)
#define GPIO0_DB_RSTN_BIT		(4)
#define GPIO1_DB_RSTN_BIT		(5)
#define PIXEL_RSTN_BIT			(6)
#define PWM_RSTN_BIT			(7)
#define SPI0_RSTN_BIT			(8)
#define SPI1_RSTN_BIT			(9)
#define I2C0_RSTN_BIT			(10)
#define I2C1_RSTN_BIT			(11)
#define ACODEC_RSTN_BIT			(12)
#define I2S_RSTN_BIT			(13)
#define UART0_RSTN_BIT			(14)
#define UART1_RSTN_BIT			(15)
#define SADC_RSTN_BIT			(16)
#define ADAPT_RSTN_BIT			(17)
#define TMR_RSTN_BIT			(18)
#define UART2_RSTN_BIT			(19)
#define SPI2_RSTN_BIT			(20)
#define JPG_ADAPT_RSTN_BIT		(21)
#define ARC_RSTN_BIT			(22)
#define EFUSE_RSTN_BIT			(23)
#define JPG_RSTN_BIT			(24)
#define VEU_RSTN_BIT			(25)
#define VPU_RSTN_BIT			(26)
#define ISP_RSTN_BIT			(27)
#define BGM_RSTN_BIT			(28)
#define I2C2_RSTN_BIT			(29)
#define EPHY_RSTN_BIT			(30)
#define SYS_RSTN_BIT			(31)

/* SWRST_AHB_CTRL */
#define EMC_HRSTN_BIT			(0)
#define SDC1_HRSTN_BIT			(1)
#define SDC0_HRSTN_BIT			(2)
#define AES_HRSTN_BIT			(3)
#define DMAC0_HRSTN_BIT			(4)
#define INTC_HRSTN_BIT			(5)
#define JPEG_ADAPT_HRSTN_BIT	(7)
#define JPEG_HRSTN_BIT			(8)
#define VCU_HRSTN_BIT			(9)
#define VPU_HRSTN_BIT			(10)
#define ISP_HRSTN_BIT			(11)
#define USB_HRSTN_BIT			(12)
#define HRSTN_BIT				(13)
#define EMAC_HRSTN_BIT			(17)
#define DDRC_HRSTN_BIT		    (19)
#define DMAC1_HRSTN_BIT			(20)
#define BGM_HRSTN_BIT			(22)
#define ADAPT_HRSTN_BIT			(23)

/* SWRST_APB_CTRL */
#define ACODEC_PRSTN_BIT		(0)
#define I2S_PRSTN_BIT			(1)
#define UART1_PRSTN_BIT			(2)
#define UART0_PRSTN_BIT			(3)
#define SPI0_PRSTN_BIT			(4)
#define SPI1_PRSTN_BIT			(5)
#define GPIO0_PRSTN_BIT			(6)
#define UART2_PRSTN_BIT			(7)
#define I2C2_PRSTN_BIT			(8)
#define I2C0_PRSTN_BIT			(9)
#define I2C1_PRSTN_BIT			(10)
#define TMR_PRSTN_BIT			(11)
#define PWM_PRSTN_BIT			(12)
#define MIPIW_PRSTN_BIT			(13)
#define MIPIC_PRSTN_BIT			(14)
#define RTC_PRSTN_BIT			(15)
#define SADC_PRSTN_BIT      	(16)
#define EFUSE_PRSTN_BIT			(17)
#define SPI2_PRSTN_BIT      	(18)
#define WDT_PRSTN_BIT      	    (19)
#define GPIO1_PRSTN_BIT			(20)

/* timer clk  fpga 1M,soc 50M*/
#ifdef CONFIG_FPGA
#define TIMER_CLK			(1000000)
#else
#define TIMER_CLK			(50000000)
#endif

#define UART1_TX_HW_HANDSHAKE   (9)
#define UART1_RX_HW_HANDSHAKE   (8)
#define UART2_TX_HW_HANDSHAKE   (13)
#define UART2_RX_HW_HANDSHAKE   (12)
#define UART1_DMA_TX_CHAN       (4)
#define UART1_DMA_RX_CHAN       (5)
#define UART2_DMA_TX_CHAN       (4)
#define UART2_DMA_RX_CHAN       (5)

/*sdio*/
#define SIMPLE_0     (0)
#define SIMPLE_22    (1)
#define SIMPLE_45    (2)
#define SIMPLE_67    (3)
#define SIMPLE_90    (4)
#define SIMPLE_112   (5)
#define SIMPLE_135   (6)
#define SIMPLE_157   (7)
#define SIMPLE_180   (8)
#define SIMPLE_202   (9)
#define SIMPLE_225   (10)
#define SIMPLE_247   (11)
#define SIMPLE_270   (12)
#define SIMPLE_292   (13)
#define SIMPLE_315   (14)
#define SIMPLE_337   (15)



#define SDIO0_RST_BIT       (~UL(1<<2))
#define SDIO0_CLK_RATE      (50000000)
#define SDIO0_CLK_DRV_SHIFT (20)
#define SDIO0_CLK_DRV_DEGREE (SIMPLE_180)
#define SDIO0_CLK_SAM_SHIFT (16)
#define SDIO0_CLK_SAM_DEGREE (SIMPLE_0)


#define SDIO1_RST_BIT       (~UL(1<<1))
#define SDIO1_CLK_RATE      (50000000)
#define SDIO1_CLK_DRV_SHIFT (12)
#define SDIO1_CLK_DRV_DEGREE (SIMPLE_180)
#define SDIO1_CLK_SAM_SHIFT (8)
#define SDIO1_CLK_SAM_DEGREE (SIMPLE_0)

#define SDC0_HRSTN  (0x1<<2)
#define SDC1_HRSTN  (0x1<<1)
#define SDC2_HRSTN  (0)


/*usb*/
#define IRQ_UHOST          USBC_IRQ
#define FH_PA_OTG          USBC_REG_BASE
#define IRQ_OTG            IRQ_UHOST
#define FH_SZ_USBHOST	   SZ_1M
#define FH_SZ_OTG          SZ_1M

#define USB_UTMI_RST_BIT      (0x1<<1)
#define USB_PHY_RST_BIT       (0x11)
#define USB_SLEEP_MODE_BIT    (0x1<<24)
#define USB_IDDQ_PWR_BIT    (0x1<<10)


/* Specific Uart Number */
#define FH_UART_NUMBER 3
#define CLK_SCAN_BIT_POS                (28)
#define INSIDE_PHY_ENABLE_BIT_POS       (24)
#define MAC_REF_CLK_DIV_MASK            (0x0f)
#define MAC_REF_CLK_DIV_BIT_POS         (24)
#define MAC_PAD_RMII_CLK_MASK           (0x0f)
#define MAC_PAD_RMII_CLK_BIT_POS        (24)
#define MAC_PAD_MAC_REF_CLK_BIT_POS     (28)
#define ETH_REF_CLK_OUT_GATE_BIT_POS    (25)
#define ETH_RMII_CLK_OUT_GATE_BIT_POS   (28)
#define IN_OR_OUT_PHY_SEL_BIT_POS       (26)
#define INSIDE_CLK_GATE_BIT_POS         (0)
#define INSIDE_PHY_SHUTDOWN_BIT_POS     (31)
#define INSIDE_PHY_RST_BIT_POS          (30)
#define INSIDE_PHY_TRAINING_BIT_POS     (27)
#define INSIDE_PHY_TRAINING_MASK        (0x0f)

#define TRAINING_EFUSE_ACTIVE_BIT_POS          4

#endif /* __ASM_ARCH_HL_H */
