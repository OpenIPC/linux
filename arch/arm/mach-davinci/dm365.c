/*
 * TI DaVinci DM365 chip specific setup
 *
 * Copyright (C) 2009 Texas Instruments
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
#include <linux/init.h>
#include <linux/clk.h>
#include <linux/serial_8250.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/spi/spi.h>
#include <linux/platform_data/edma.h>

#include <asm/mach/map.h>

#include <mach/cputype.h>
#include <mach/psc.h>
#include <mach/mux.h>
#include <mach/irqs.h>
#include <mach/time.h>
#include <mach/serial.h>
#include <mach/common.h>
#include <linux/platform_data/keyscan-davinci.h>
#include <linux/platform_data/spi-davinci.h>
#include <mach/gpio-davinci.h>
#include "davinci.h"

#include "clock.h"
#include "mux.h"
#include "asp.h"


#define DM365_REF_FREQ		24000000	/* 24 MHz on the DM365 EVM */
#define DM365_RTC_BASE			0x01c69000
#define DM365_KEYSCAN_BASE		0x01c69400
#define DM365_OSD_BASE			0x01c71c00
#define DM365_VENC_BASE			0x01c71e00
#define DAVINCI_DM365_VC_BASE		0x01d0c000
#define DAVINCI_DMA_VC_TX		2
#define DAVINCI_DMA_VC_RX		3
#define DM365_EMAC_BASE			0x01d07000
#define DM365_EMAC_MDIO_BASE		(DM365_EMAC_BASE + 0x4000)
#define DM365_EMAC_CNTRL_OFFSET		0x0000
#define DM365_EMAC_CNTRL_MOD_OFFSET	0x3000
#define DM365_EMAC_CNTRL_RAM_OFFSET	0x1000
#define DM365_EMAC_CNTRL_RAM_SIZE	0x2000

static struct pll_data pll1_data = {
	.num		= 1,
	.phys_base	= DAVINCI_PLL1_BASE,
	.flags		= PLL_HAS_POSTDIV | PLL_HAS_PREDIV,
};

static struct pll_data pll2_data = {
	.num		= 2,
	.phys_base	= DAVINCI_PLL2_BASE,
	.flags		= PLL_HAS_POSTDIV | PLL_HAS_PREDIV,
};

static struct clk ref_clk = {
	.name		= "ref_clk",
	.rate		= DM365_REF_FREQ,
};

static struct clk pll1_clk = {
	.name		= "pll1",
	.parent		= &ref_clk,
	.flags		= CLK_PLL,
	.pll_data	= &pll1_data,
};

static struct clk pll1_aux_clk = {
	.name		= "pll1_aux_clk",
	.parent		= &pll1_clk,
	.flags		= CLK_PLL | PRE_PLL,
};

static struct clk pll1_sysclkbp = {
	.name		= "pll1_sysclkbp",
	.parent		= &pll1_clk,
	.flags 		= CLK_PLL | PRE_PLL,
	.div_reg	= BPDIV
};

static struct clk clkout0_clk = {
	.name		= "clkout0",
	.parent		= &pll1_clk,
	.flags		= CLK_PLL | PRE_PLL,
};

static struct clk pll1_sysclk1 = {
	.name		= "pll1_sysclk1",
	.parent		= &pll1_clk,
	.flags		= CLK_PLL,
	.div_reg	= PLLDIV1,
};

static struct clk pll1_sysclk2 = {
	.name		= "pll1_sysclk2",
	.parent		= &pll1_clk,
	.flags		= CLK_PLL,
	.div_reg	= PLLDIV2,
};

static struct clk pll1_sysclk3 = {
	.name		= "pll1_sysclk3",
	.parent		= &pll1_clk,
	.flags		= CLK_PLL,
	.div_reg	= PLLDIV3,
};

static struct clk pll1_sysclk4 = {
	.name		= "pll1_sysclk4",
	.parent		= &pll1_clk,
	.flags		= CLK_PLL,
	.div_reg	= PLLDIV4,
};

static struct clk pll1_sysclk5 = {
	.name		= "pll1_sysclk5",
	.parent		= &pll1_clk,
	.flags		= CLK_PLL,
	.div_reg	= PLLDIV5,
};

static struct clk pll1_sysclk6 = {
	.name		= "pll1_sysclk6",
	.parent		= &pll1_clk,
	.flags		= CLK_PLL,
	.div_reg	= PLLDIV6,
};

static struct clk pll1_sysclk7 = {
	.name		= "pll1_sysclk7",
	.parent		= &pll1_clk,
	.flags		= CLK_PLL,
	.div_reg	= PLLDIV7,
};

static struct clk pll1_sysclk8 = {
	.name		= "pll1_sysclk8",
	.parent		= &pll1_clk,
	.flags		= CLK_PLL,
	.div_reg	= PLLDIV8,
};

static struct clk pll1_sysclk9 = {
	.name		= "pll1_sysclk9",
	.parent		= &pll1_clk,
	.flags		= CLK_PLL,
	.div_reg	= PLLDIV9,
};

static struct clk pll2_clk = {
	.name		= "pll2",
	.parent		= &ref_clk,
	.flags		= CLK_PLL,
	.pll_data	= &pll2_data,
};

static struct clk pll2_aux_clk = {
	.name		= "pll2_aux_clk",
	.parent		= &pll2_clk,
	.flags		= CLK_PLL | PRE_PLL,
};

static struct clk clkout1_clk = {
	.name		= "clkout1",
	.parent		= &pll2_clk,
	.flags		= CLK_PLL | PRE_PLL,
};

static struct clk pll2_sysclk1 = {
	.name		= "pll2_sysclk1",
	.parent		= &pll2_clk,
	.flags		= CLK_PLL,
	.div_reg	= PLLDIV1,
};

static struct clk pll2_sysclk2 = {
	.name		= "pll2_sysclk2",
	.parent		= &pll2_clk,
	.flags		= CLK_PLL,
	.div_reg	= PLLDIV2,
};

static struct clk pll2_sysclk3 = {
	.name		= "pll2_sysclk3",
	.parent		= &pll2_clk,
	.flags		= CLK_PLL,
	.div_reg	= PLLDIV3,
};

static struct clk pll2_sysclk4 = {
	.name		= "pll2_sysclk4",
	.parent		= &pll2_clk,
	.flags		= CLK_PLL,
	.div_reg	= PLLDIV4,
};

static struct clk pll2_sysclk5 = {
	.name		= "pll2_sysclk5",
	.parent		= &pll2_clk,
	.flags		= CLK_PLL,
	.div_reg	= PLLDIV5,
};

static struct clk pll2_sysclk6 = {
	.name		= "pll2_sysclk6",
	.parent		= &pll2_clk,
	.flags		= CLK_PLL,
	.div_reg	= PLLDIV6,
};

static struct clk pll2_sysclk7 = {
	.name		= "pll2_sysclk7",
	.parent		= &pll2_clk,
	.flags		= CLK_PLL,
	.div_reg	= PLLDIV7,
};

static struct clk pll2_sysclk8 = {
	.name		= "pll2_sysclk8",
	.parent		= &pll2_clk,
	.flags		= CLK_PLL,
	.div_reg	= PLLDIV8,
};

static struct clk pll2_sysclk9 = {
	.name		= "pll2_sysclk9",
	.parent		= &pll2_clk,
	.flags		= CLK_PLL,
	.div_reg	= PLLDIV9,
};

static struct clk vpss_dac_clk = {
	.name		= "vpss_dac",
	.parent		= &pll1_sysclk3,
	.lpsc		= DM365_LPSC_DAC_CLK,
	.flags		= ALWAYS_ENABLED,
};

static struct clk vpss_master_clk = {
	.name		= "vpss_master",
	.parent		= &pll1_sysclk5,
	.lpsc		= DM365_LPSC_VPSSMSTR,
	.flags		= CLK_PSC,
};

/*static struct clk vpss_slave_clk = {
	.name		= "vpss_slave",
	.parent		= &pll1_sysclk5,
	.lpsc		= DAVINCI_LPSC_VPSSSLV,
};*/

static struct clk arm_clk = {
	.name		= "arm_clk",
	.parent		= &pll2_sysclk2,
	.lpsc		= DAVINCI_LPSC_ARM,
	.flags		= ALWAYS_ENABLED,
};

static struct clk uart0_clk = {
	.name		= "uart0",
	.parent		= &pll1_aux_clk,
	.lpsc		= DAVINCI_LPSC_UART0,
};

static struct clk uart1_clk = {
	.name		= "uart1",
	.parent		= &pll1_sysclk4,
	.lpsc		= DAVINCI_LPSC_UART1,
};

static struct clk i2c_clk = {
	.name		= "i2c",
	.parent		= &pll1_aux_clk,
	.lpsc		= DAVINCI_LPSC_I2C,
};

static struct clk mmcsd0_clk = {
	.name		= "mmcsd0",
	.parent		= &pll1_sysclk8,
	.lpsc		= DAVINCI_LPSC_MMC_SD,
};

static struct clk mmcsd1_clk = {
	.name		= "mmcsd1",
	.parent		= &pll1_sysclk4,
	.lpsc		= DM365_LPSC_MMC_SD1,
};

static struct clk spi0_clk = {
	.name		= "spi0",
	.parent		= &pll1_sysclk4,
	.lpsc		= DAVINCI_LPSC_SPI,
};

static struct clk spi1_clk = {
	.name		= "spi1",
	.parent		= &pll1_sysclk4,
	.lpsc		= DM365_LPSC_SPI1,
};

static struct clk spi2_clk = {
	.name		= "spi2",
	.parent		= &pll1_sysclk4,
	.lpsc		= DM365_LPSC_SPI2,
};

static struct clk spi3_clk = {
	.name		= "spi3",
	.parent		= &pll1_sysclk4,
	.lpsc		= DM365_LPSC_SPI3,
};

static struct clk spi4_clk = {
	.name		= "spi4",
	.parent		= &pll1_aux_clk,
	.lpsc		= DM365_LPSC_SPI4,
};

static struct clk gpio_clk = {
	.name		= "gpio",
	.parent		= &pll1_sysclk4,
	.lpsc		= DAVINCI_LPSC_GPIO,
};

static struct clk aemif_clk = {
	.name		= "aemif",
	.parent		= &pll1_sysclk4,
	.lpsc		= DAVINCI_LPSC_AEMIF,
};

static struct clk pwm0_clk = {
	.name		= "pwm0",
	.parent		= &pll1_aux_clk,
	.lpsc		= DAVINCI_LPSC_PWM0,
};

static struct clk pwm1_clk = {
	.name		= "pwm1",
	.parent		= &pll1_aux_clk,
	.lpsc		= DAVINCI_LPSC_PWM1,
};

static struct clk pwm2_clk = {
	.name		= "pwm2",
	.parent		= &pll1_aux_clk,
	.lpsc		= DAVINCI_LPSC_PWM2,
};

static struct clk pwm3_clk = {
	.name		= "pwm3",
	.parent		= &pll1_aux_clk,
	.lpsc		= DM365_LPSC_PWM3,
};
/*
static struct clk rto_clk = {
	.name		= "rto",
	.parent		= &pll1_aux_clk,
	.lpsc		= DM365_LPSC_RTO,
};
*/
static struct clk timer0_clk = {
	.name		= "timer0",
	.parent		= &pll1_aux_clk,
	.lpsc		= DAVINCI_LPSC_TIMER0,
};

static struct clk timer1_clk = {
	.name		= "timer1",
	.parent		= &pll1_aux_clk,
	.lpsc		= DAVINCI_LPSC_TIMER1,
};

static struct clk timer2_clk = {
	.name		= "timer2",
	.parent		= &pll1_aux_clk,
	.lpsc		= DAVINCI_LPSC_TIMER2,
	.usecount	= 1,
};

static struct clk timer3_clk = {
	.name		= "timer3",
	.parent		= &pll1_aux_clk,
	.lpsc		= DM365_LPSC_TIMER3,
};

static struct clk usb_clk = {
	.name		= "usb",
	.parent		= &pll1_aux_clk,
	.lpsc		= DAVINCI_LPSC_USB,
};

static struct clk emac_clk = {
	.name		= "emac",
	.parent		= &pll1_sysclk4,
	.lpsc		= DM365_LPSC_EMAC,
};

static struct clk voicecodec_clk = {
	.name		= "voice_codec",
	.parent		= &pll2_sysclk4,
	.lpsc		= DM365_LPSC_VOICE_CODEC,
};

static struct clk asp0_clk = {
	.name		= "asp0",
	.parent		= &pll1_sysclk4,
	.lpsc		= DM365_LPSC_McBSP1,
};

static struct clk rto_clk = {
	.name		= "rto",
	.parent		= &pll1_sysclk4,
	.lpsc		= DM365_LPSC_RTO,
};

static struct clk mjcp_clk = {
	.name		= "mjcp",
	.parent		= &pll1_sysclk3,
	.lpsc		= DM365_LPSC_MJCP,
};

static struct clk_lookup dm365_clks[] = {
	CLK(NULL, "ref", &ref_clk),
	CLK(NULL, "pll1", &pll1_clk),
	CLK(NULL, "pll1_aux", &pll1_aux_clk),
	CLK(NULL, "pll1_sysclkbp", &pll1_sysclkbp),
	CLK(NULL, "clkout0", &clkout0_clk),
	CLK(NULL, "pll1_sysclk1", &pll1_sysclk1),
	CLK(NULL, "pll1_sysclk2", &pll1_sysclk2),
	CLK(NULL, "pll1_sysclk3", &pll1_sysclk3),
	CLK(NULL, "pll1_sysclk4", &pll1_sysclk4),
	CLK(NULL, "pll1_sysclk5", &pll1_sysclk5),
	CLK(NULL, "pll1_sysclk6", &pll1_sysclk6),
	CLK(NULL, "pll1_sysclk7", &pll1_sysclk7),
	CLK(NULL, "pll1_sysclk8", &pll1_sysclk8),
	CLK(NULL, "pll1_sysclk9", &pll1_sysclk9),
	CLK(NULL, "pll2", &pll2_clk),
	CLK(NULL, "pll2_aux", &pll2_aux_clk),
	CLK(NULL, "clkout1", &clkout1_clk),
	CLK(NULL, "pll2_sysclk1", &pll2_sysclk1),
	CLK(NULL, "pll2_sysclk2", &pll2_sysclk2),
	CLK(NULL, "pll2_sysclk3", &pll2_sysclk3),
	CLK(NULL, "pll2_sysclk4", &pll2_sysclk4),
	CLK(NULL, "pll2_sysclk5", &pll2_sysclk5),
	CLK(NULL, "pll2_sysclk6", &pll2_sysclk6),
	CLK(NULL, "pll2_sysclk7", &pll2_sysclk7),
	CLK(NULL, "pll2_sysclk8", &pll2_sysclk8),
	CLK(NULL, "pll2_sysclk9", &pll2_sysclk9),
	CLK(NULL, "vpss_dac", &vpss_dac_clk),
	CLK(NULL, "vpss_master", &vpss_master_clk),
	CLK(NULL, "arm", &arm_clk),
	CLK(NULL, "uart0", &uart0_clk),
	CLK(NULL, "uart1", &uart1_clk),
	CLK("i2c_davinci.1", NULL, &i2c_clk),
	CLK("dm365-mmc.0", NULL, &mmcsd0_clk),
	CLK("dm365-mmc.1", NULL, &mmcsd1_clk),
	CLK("spi_davinci.0", NULL, &spi0_clk),
	CLK("spi_davinci.1", NULL, &spi1_clk),
	CLK("spi_davinci.2", NULL, &spi2_clk),
	CLK("spi_davinci.3", NULL, &spi3_clk),
	CLK("spi_davinci.4", NULL, &spi4_clk),
	CLK(NULL, "gpio", &gpio_clk),
	CLK(NULL, "aemif", &aemif_clk),
	CLK(NULL, "pwm0", &pwm0_clk),
	CLK(NULL, "pwm1", &pwm1_clk),
	CLK(NULL, "pwm2", &pwm2_clk),
	CLK(NULL, "pwm3", &pwm3_clk),
	//CLK(NULL, "rto", &rto_clk),
	CLK(NULL, "timer0", &timer0_clk),
	CLK(NULL, "timer1", &timer1_clk),
	CLK("watchdog", NULL, &timer2_clk),
	CLK(NULL, "timer3", &timer3_clk),
	CLK(NULL, "usb", &usb_clk),
	CLK("davinci_emac.1", NULL, &emac_clk),
	CLK("davinci_voicecodec", NULL, &voicecodec_clk),
	CLK("davinci-mcbsp", NULL, &asp0_clk),
	CLK(NULL, "rto", &rto_clk),
	CLK(NULL, "mjcp", &mjcp_clk),
	CLK(NULL, NULL, NULL),
};

/*----------------------------------------------------------------------*/

#define PINMUX0		0x00
#define PINMUX1		0x04
#define PINMUX2		0x08
#define PINMUX3		0x0c
#define PINMUX4		0x10
#define INTMUX		0x18
#define EVTMUX		0x1c


static const struct mux_config dm365_pins[] = {
#ifdef CONFIG_DAVINCI_MUX
MUX_CFG(DM365,	MMCSD0,		0,   24,     1,	  0,	 false)

MUX_CFG(DM365,	SD1_CLK,	0,   16,    3,	  1,	 false)
MUX_CFG(DM365,	SD1_CMD,	4,   30,    3,	  1,	 false)
MUX_CFG(DM365,	SD1_DATA3,	4,   28,    3,	  1,	 false)
MUX_CFG(DM365,	SD1_DATA2,	4,   26,    3,	  1,	 false)
MUX_CFG(DM365,	SD1_DATA1,	4,   24,    3,	  1,	 false)
MUX_CFG(DM365,	SD1_DATA0,	4,   22,    3,	  1,	 false)

MUX_CFG(DM365,	I2C_SDA,	3,   23,    3,	  2,	 false)
MUX_CFG(DM365,	I2C_SCL,	3,   21,    3,	  2,	 false)

MUX_CFG(DM365,	AEMIF_CE0,	2,   7,     1,	  0,	 false)
MUX_CFG(DM365,	AEMIF_CE1,	2,   8,     1,	  0,	 false)


MUX_CFG(DM365,	SPI0_SCLK,	3,   28,    1,    1,	 false)
MUX_CFG(DM365,	SPI0_SDI,	3,   26,    3,    1,	 false)
MUX_CFG(DM365,	SPI0_SDO,	3,   25,    1,    1,	 false)
MUX_CFG(DM365,	SPI0_SDENA0,	3,   29,    3,    1,	 false)
MUX_CFG(DM365,	SPI0_SDENA1,	3,   26,    3,    2,	 false)

MUX_CFG(DM365,	UART0_RXD,	3,   20,    1,    1,	 false)
MUX_CFG(DM365,	UART0_TXD,	3,   19,    1,    1,	 false)
MUX_CFG(DM365,	UART1_RXD,	3,   17,    3,    2,	 false)
MUX_CFG(DM365,	UART1_TXD,	3,   15,    3,    2,	 false)
MUX_CFG(DM365,	UART1_RTS,	3,   23,    3,    1,	 false)
MUX_CFG(DM365,	UART1_CTS,	3,   21,    3,    1,	 false)

MUX_CFG(DM365,  EMAC_TX_EN,	3,   17,    3,    1,     false)
MUX_CFG(DM365,  EMAC_TX_CLK,	3,   15,    3,    1,     false)
MUX_CFG(DM365,  EMAC_COL,	3,   14,    1,    1,     false)
MUX_CFG(DM365,  EMAC_TXD3,	3,   13,    1,    1,     false)
MUX_CFG(DM365,  EMAC_TXD2,	3,   12,    1,    1,     false)
MUX_CFG(DM365,  EMAC_TXD1,	3,   11,    1,    1,     false)
MUX_CFG(DM365,  EMAC_TXD0,	3,   10,    1,    1,     false)
MUX_CFG(DM365,  EMAC_RXD3,	3,   9,     1,    1,     false)
MUX_CFG(DM365,  EMAC_RXD2,	3,   8,     1,    1,     false)
MUX_CFG(DM365,  EMAC_RXD1,	3,   7,     1,    1,     false)
MUX_CFG(DM365,  EMAC_RXD0,	3,   6,     1,    1,     false)
MUX_CFG(DM365,  EMAC_RX_CLK,	3,   5,     1,    1,     false)
MUX_CFG(DM365,  EMAC_RX_DV,	3,   4,     1,    1,     false)
MUX_CFG(DM365,  EMAC_RX_ER,	3,   3,     1,    1,     false)
MUX_CFG(DM365,  EMAC_CRS,	3,   2,     1,    1,     false)
MUX_CFG(DM365,  EMAC_MDIO,	3,   1,     1,    1,     false)
MUX_CFG(DM365,  EMAC_MDCLK,	3,   0,     1,    1,     false)

MUX_CFG(DM365,	PWM0,		1,   0,     3,    2,     false)
MUX_CFG(DM365,	PWM0_G23,	3,   26,    3,    3,     false)
MUX_CFG(DM365,	PWM1,		1,   2,     3,    2,     false)
MUX_CFG(DM365,	PWM1_G25,	3,   29,    3,    2,     false)
MUX_CFG(DM365,	PWM2_G87,	1,   10,    3,    2,     false)
MUX_CFG(DM365,	PWM2_G88,	1,   8,     3,    2,     false)
MUX_CFG(DM365,	PWM2_G89,	1,   6,     3,    2,     false)
MUX_CFG(DM365,	PWM2_G90,	1,   4,     3,    2,     false)
MUX_CFG(DM365,	PWM3_G80,	1,   20,    3,    3,     false)
MUX_CFG(DM365,	PWM3_G81,	1,   18,    3,    3,     false)
MUX_CFG(DM365,	PWM3_G85,	1,   14,    3,    2,     false)
MUX_CFG(DM365,	PWM3_G86,	1,   12,    3,    2,     false)

MUX_CFG(DM365,	SPI1_SCLK,	4,   2,     3,    1,	 false)
MUX_CFG(DM365,	SPI1_SDI,	3,   31,    1,    1,	 false)
MUX_CFG(DM365,	SPI1_SDO,	4,   0,     3,    1,	 false)
MUX_CFG(DM365,	SPI1_SDENA0,	4,   4,     3,    1,	 false)
MUX_CFG(DM365,	SPI1_SDENA1,	4,   0,     3,    2,	 false)

MUX_CFG(DM365,	SPI2_SCLK,	4,   10,    3,    1,	 false)
MUX_CFG(DM365,	SPI2_SDI,	4,   6,     3,    1,	 false)
MUX_CFG(DM365,	SPI2_SDO,	4,   8,     3,    1,	 false)
MUX_CFG(DM365,	SPI2_SDENA0,	4,   12,    3,    1,	 false)
MUX_CFG(DM365,	SPI2_SDENA1,	4,   8,     3,    2,	 false)

MUX_CFG(DM365,	SPI3_SCLK,	0,   0,	    3,    2,	 false)
MUX_CFG(DM365,	SPI3_SDI,	0,   2,     3,    2,	 false)
MUX_CFG(DM365,	SPI3_SDO,	0,   6,     3,    2,	 false)
MUX_CFG(DM365,	SPI3_SDENA0,	0,   4,     3,    2,	 false)
MUX_CFG(DM365,	SPI3_SDENA1,	0,   6,     3,    3,	 false)

MUX_CFG(DM365,	SPI4_SCLK,	4,   18,    3,    1,	 false)
MUX_CFG(DM365,	SPI4_SDI,	4,   14,    3,    1,	 false)
MUX_CFG(DM365,	SPI4_SDO,	4,   16,    3,    1,	 false)
MUX_CFG(DM365,	SPI4_SDENA0,	4,   20,    3,    0,	 false)
MUX_CFG(DM365,	SPI4_SDENA1,	4,   16,    3,    2,	 false)

MUX_CFG(DM365,	CLKOUT0,	4,   20,    3,    3,     false)
MUX_CFG(DM365,	GPIO37,		4,   20,    0,    0,	 false)
MUX_CFG(DM365,	CLKOUT1,	4,   16,    3,    3,     false)
MUX_CFG(DM365,	CLKOUT2,	4,   8,     3,    3,     false)

// added by Gol
MUX_CFG(DM365,	GPIO1,		3,   0,     1,    0,	 false)
MUX_CFG(DM365,	GPIO2,		3,   1,     1,    0,	 false)
MUX_CFG(DM365,	GPIO3,		3,   2,     1,    0,	 false)
MUX_CFG(DM365,	GPIO4,		3,   3,     1,    0,	 false)
MUX_CFG(DM365,	GPIO5,		3,   4,     1,    0,	 false)
MUX_CFG(DM365,	GPIO6,		3,   5,     1,    0,	 false)
MUX_CFG(DM365,	GPIO7,		3,   6,     1,    0,	 false)
MUX_CFG(DM365,	GPIO8,		3,   7,     1,    0,	 false)
MUX_CFG(DM365,	GPIO9,		3,   8,     1,    0,	 false)
MUX_CFG(DM365,	GPIO10,		3,   9,     1,    0,	 false)
MUX_CFG(DM365,	GPIO11,		3,   10,     1	,    0,	 false)
MUX_CFG(DM365,	GPIO12,		3,   11,     1,    0,	 false)
MUX_CFG(DM365,	GPIO13,		3,   12,     1,    0,	 false)
MUX_CFG(DM365,	GPIO14,		3,   13,     1,    0,	 false)
MUX_CFG(DM365,	GPIO15,		3,   14,     1,    0,	 false)
MUX_CFG(DM365,	GPIO16,		3,   15,     3,    0,	 false)
MUX_CFG(DM365,	GPIO17,		3,   17,     3,    0,	 false)
MUX_CFG(DM365,	GPIO18,		3,   19,     1,    0,	 false)
MUX_CFG(DM365,	GPIO19,		3,   20,     1,    0,	 false)

MUX_CFG(DM365,	GPIO21,		3,   23,     0,    0,	 false)

MUX_CFG(DM365,	GPIO36,		4,   18,     3,    0,	 false)
MUX_CFG(DM365,	GPIO37,		4,   20,     3,    0,	 false)
MUX_CFG(DM365,	GPIO38,		4,   22,     3,    0,	 false)
MUX_CFG(DM365,	GPIO39,		4,   24,     3,    0,	 false)
MUX_CFG(DM365,	GPIO40,		4,   26,     3,    0,	 false)
MUX_CFG(DM365,	GPIO41,		4,   28,     3,    0,	 false)
MUX_CFG(DM365,	GPIO42,		4,   30,     3,    0,	 false)
MUX_CFG(DM365,	GPIO43,		0,   16,     3,    0,	 false)

MUX_CFG(DM365,	GPIO52,		2,   10,     1,    0,	 false)
MUX_CFG(DM365,	GPIO53,		2,   9,      1,    0,	 false)
MUX_CFG(DM365,	GPIO54,		2,   9,      1,    0,	 false)
MUX_CFG(DM365,	GPIO55,		2,   8,      1,    0,	 false)
MUX_CFG(DM365,	GPIO56,		2,   7,      1,    0,	 false)

MUX_CFG(DM365,	GPIO57,		0,   0,      0,    0,	 false) // unknown
MUX_CFG(DM365,	GPIO58,		0,   0,      0,    0,	 false) // unknown
MUX_CFG(DM365,	GPIO59,		0,   0,      0,    0,	 false) // unknown
MUX_CFG(DM365,	GPIO60,		0,   0,      0,    0,	 false) // unknown
MUX_CFG(DM365,	GPIO61,		0,   0,      0,    0,	 false) // unknown
MUX_CFG(DM365,	GPIO62,		0,   0,      0,    0,	 false) // unknown
MUX_CFG(DM365,	GPIO63,		0,   0,      0,    0,	 false) // unknown
MUX_CFG(DM365,	GPIO64,		0,   0,      0,    0,	 false) // unknown

MUX_CFG(DM365,	GPIO65,		0,   0,     0,    0,	 false) // unknown

MUX_CFG(DM365,	GPIO68,		2,   2,     0,    0,	 false)
MUX_CFG(DM365,	GPIO69,		2,   0,     0,    0,	 false)
MUX_CFG(DM365,	GPIO70,		2,   0,     0,    0,	 false)
MUX_CFG(DM365,	GPIO71,		2,   0,     0,    0,	 false)

MUX_CFG(DM365,	GPIO72,		2,   4,     0,    0,	 false)

MUX_CFG(DM365,	GPIO73,		2,   0,     0,    0,	 false) // unknown
MUX_CFG(DM365,	GPIO74,		2,   0,     0,    0,	 false) // unknown
MUX_CFG(DM365,	GPIO75,		2,   0,     0,    0,	 false) // unknown
MUX_CFG(DM365,	GPIO76,		2,   0,     0,    0,	 false) // unknown
MUX_CFG(DM365,	GPIO77,		2,   0,     0,    0,	 false) // unknown
MUX_CFG(DM365,	GPIO78,		2,   0,     0,    0,	 false) // unknown

MUX_CFG(DM365,	GPIO93,		0,   14,    1,    0,	 false)
MUX_CFG(DM365,	GPIO94,		0,   13,    1,    0,	 false)
MUX_CFG(DM365,	GPIO95,		0,   12,    1,    0,	 false)
MUX_CFG(DM365,	GPIO96,		0,   11,    1,    0,	 false)
MUX_CFG(DM365,	GPIO97,		0,   10,    1,    0,	 false)
MUX_CFG(DM365,	GPIO98,		0,   9,    1,    0,	 false)
MUX_CFG(DM365,	GPIO99,		0,   8,    1,    0,	 false)



// end added by Gol

MUX_CFG(DM365,	GPIO20,		3,   21,    3,    0,	 false)
MUX_CFG(DM365,	GPIO22,		3,   25,    1,    0,	 false)
MUX_CFG(DM365,	GPIO23,		3,   26,    3,    0,	 false)
MUX_CFG(DM365,	GPIO24,		3,   28,    1,    0,	 false)
MUX_CFG(DM365,	GPIO25,		3,   29,    3,    0,	 false)
MUX_CFG(DM365,	GPIO26,		3,   31,    1,    0,	 false)
MUX_CFG(DM365,	GPIO27,		4,    0,    3,    0,	 false)
MUX_CFG(DM365,	GPIO28,		4,    2,    3,    0,	 false)
MUX_CFG(DM365,	GPIO29,		4,    4,    3,    0,	 false)
MUX_CFG(DM365,	GPIO30,		4,    6,    3,    0,	 false)
MUX_CFG(DM365,	GPIO31,		4,    8,    3,    0,	 false)
MUX_CFG(DM365,	GPIO32,		4,   10,    3,    0,	 false)
MUX_CFG(DM365,	GPIO33,		4,   12,    3,    0,	 false)
MUX_CFG(DM365,	GPIO34,		4,   14,    3,    0,	 false)
MUX_CFG(DM365,	GPIO35,		4,   16,    3,    0,	 false)
MUX_CFG(DM365,	GPIO44,		0,   18,    1,    0,	 false)
MUX_CFG(DM365,	GPIO45,		0,   19,    1,    0,	 false)
MUX_CFG(DM365,	GPIO46,		0,   20,    1,    0,	 false)
MUX_CFG(DM365,	GPIO47,		0,   21,    1,    0,	 false)
MUX_CFG(DM365,	GPIO48,		0,   22,    1,    0,	 false)
MUX_CFG(DM365,	GPIO49,		0,   23,    1,    0,	 false)
MUX_CFG(DM365,	GPIO50,		2,   12,    1,    1,	 false)
MUX_CFG(DM365,	GPIO51,		2,   11,    1,    1,	 false)
MUX_CFG(DM365,	GPIO66,		2,   0,     3,	  0,	 false)
MUX_CFG(DM365,	GPIO67,		2,    0,    3,    0,	 false)
MUX_CFG(DM365,	GPIO79,		1,   22,    1,    1,	 false)
MUX_CFG(DM365,	GPIO80,		1,   20,    3,    0,	 false)
MUX_CFG(DM365,	GPIO81,		1,   18,    3,    0,	 false)
MUX_CFG(DM365,	GPIO82,		1,   17,    1,    1,	 false)
MUX_CFG(DM365,	GPIO83,		1,   16,    1,    1,	 false)
MUX_CFG(DM365,	GPIO84,		1,   16,    1,    1,	 false)
MUX_CFG(DM365,	GPIO85,		1,   14,    3,    0,	 false)
MUX_CFG(DM365,	GPIO86,		1,   12,    3,    0,	 false)
MUX_CFG(DM365,	GPIO87,		1,   10,    3,    0,	 false)
MUX_CFG(DM365,	GPIO88,		1,    8,    3,    0,	 false)
MUX_CFG(DM365,	GPIO89,		1,    6,    3,    0,	 false)

MUX_CFG(DM365,	GPIO90,		1,    4,    3,    0,	 false)
//added by dlinyj
//(soc, desc, muxreg, mode_offset, mode_mask, mux_mode, dbg)
MUX_CFG(DM365,	RTO0,		1,    4,    3,    3,	 false)
MUX_CFG(DM365,	RTO1,		1,    6,    3,    3,	 false)
MUX_CFG(DM365,	RTO2,		1,    8,    3,    3,	 false)
MUX_CFG(DM365,	RTO3,		1,    10,    3,    3,	 false)
//end added by dlinyj
MUX_CFG(DM365,	GPIO91,		1,    2,    3,    0,	 false)
MUX_CFG(DM365,	GPIO92,		1,    0,    3,    0,	 false)
MUX_CFG(DM365,	GPIO100,	0,    6,    3,    1,	 false)
MUX_CFG(DM365,	GPIO101,	0,    4,    3,    1,	 false)
MUX_CFG(DM365,	GPIO102,	0,    2,    3,    1,	 false)
MUX_CFG(DM365,	GPIO103,	0,    0,    3,    1,	 false)

MUX_CFG(DM365,	VIN_CAM_VD,	0,   13,    1,	  0,	 false)
MUX_CFG(DM365,	VIN_CAM_HD,	0,   12,    1,	  0,	 false)
MUX_CFG(DM365,  EXTCLK,         0,   14,    0x03, 2,     false)//GPIO93 - CAMERA CLK
MUX_CFG(DM365,	CAM_OFF,	0,   9,    1,	  1,	 false)
MUX_CFG(DM365,	CAM_RESET,	0,   8,    1,	  1,	 false)

INT_CFG(DM365,  INT_EDMA_CC,         2,     1,    1,     false)
INT_CFG(DM365,  INT_EDMA_TC0_ERR,    3,     1,    1,     false)
INT_CFG(DM365,  INT_EDMA_TC1_ERR,    4,     1,    1,     false)
INT_CFG(DM365,  INT_EDMA_TC2_ERR,    22,    1,    1,     false)
INT_CFG(DM365,  INT_EDMA_TC3_ERR,    23,    1,    1,     false)
INT_CFG(DM365,  INT_PRTCSS,          10,    1,    1,     false)
INT_CFG(DM365,  INT_EMAC_RXTHRESH,   14,    1,    1,     false)
INT_CFG(DM365,  INT_EMAC_RXPULSE,    15,    1,    1,     false)
INT_CFG(DM365,  INT_EMAC_TXPULSE,    16,    1,    1,     false)
INT_CFG(DM365,  INT_EMAC_MISCPULSE,  17,    1,    1,     false)
INT_CFG(DM365,  INT_IMX0_ENABLE,     0,     1,    0,     false)
INT_CFG(DM365,  INT_IMX0_DISABLE,    0,     1,    1,     false)
INT_CFG(DM365,  INT_HDVICP_ENABLE,   0,     1,    1,     false)
INT_CFG(DM365,  INT_HDVICP_DISABLE,  0,     1,    0,     false)
INT_CFG(DM365,  INT_IMX1_ENABLE,     24,    1,    1,     false)
INT_CFG(DM365,  INT_IMX1_DISABLE,    24,    1,    0,     false)
INT_CFG(DM365,  INT_NSF_ENABLE,      25,    1,    1,     false)
INT_CFG(DM365,  INT_NSF_DISABLE,     25,    1,    0,     false)
INT_CFG(DM365,  INT_VCIF_ENABLE,     7,     1,    1,     false)
INT_CFG(DM365,  INT_VCIF_DISABLE,    7,     1,    0,     false)
INT_CFG(DM365,  INT_SPI3,            13,    1,    1,     false)

EVT_CFG(DM365,	EVT2_ASP_TX,         0,     1,    0,     false)
EVT_CFG(DM365,	EVT3_ASP_RX,         1,     1,    0,     false)
EVT_CFG(DM365,	EVT2_VC_TX,          0,     1,    1,     false)
EVT_CFG(DM365,	EVT3_VC_RX,          1,     1,    1,     false)
EVT_CFG(DM365,	EVT18_SPI3_TX, 3,     1,    1,     false)
EVT_CFG(DM365,	EVT19_SPI3_RX, 4,     1,    1,     false)
#endif
};


static struct emac_platform_data dm365_emac_pdata = {
	.ctrl_reg_offset	= DM365_EMAC_CNTRL_OFFSET,
	.ctrl_mod_reg_offset	= DM365_EMAC_CNTRL_MOD_OFFSET,
	.ctrl_ram_offset	= DM365_EMAC_CNTRL_RAM_OFFSET,
	.ctrl_ram_size		= DM365_EMAC_CNTRL_RAM_SIZE,
	.version		= EMAC_VERSION_2,
};

static struct resource dm365_emac_resources[] = {
	{
		.start	= DM365_EMAC_BASE,
		.end	= DM365_EMAC_BASE + SZ_16K - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= IRQ_DM365_EMAC_RXTHRESH,
		.end	= IRQ_DM365_EMAC_RXTHRESH,
		.flags	= IORESOURCE_IRQ,
	},
	{
		.start	= IRQ_DM365_EMAC_RXPULSE,
		.end	= IRQ_DM365_EMAC_RXPULSE,
		.flags	= IORESOURCE_IRQ,
	},
	{
		.start	= IRQ_DM365_EMAC_TXPULSE,
		.end	= IRQ_DM365_EMAC_TXPULSE,
		.flags	= IORESOURCE_IRQ,
	},
	{
		.start	= IRQ_DM365_EMAC_MISCPULSE,
		.end	= IRQ_DM365_EMAC_MISCPULSE,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device dm365_emac_device = {
	.name		= "davinci_emac",
	.id		= 1,
	.dev = {
		.platform_data	= &dm365_emac_pdata,
	},
	.num_resources	= ARRAY_SIZE(dm365_emac_resources),
	.resource	= dm365_emac_resources,
};
static struct resource dm365_mdio_resources[] = {
	{
		.start	= DM365_EMAC_MDIO_BASE,
		.end	= DM365_EMAC_MDIO_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	},
};

static struct platform_device dm365_mdio_device = {
	.name		= "davinci_mdio",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(dm365_mdio_resources),
	.resource	= dm365_mdio_resources,
};



/* IPIPEIF device configuration */
static u64 dm365_ipipeif_dma_mask = DMA_BIT_MASK(32);
static struct resource dm365_ipipeif_resources[] = {
	{
		.start          = 0x01C71200,
		.end            = 0x01C71200 + 0x60,
		.flags          = IORESOURCE_MEM,
	},
};

static struct platform_device dm365_ipipeif_dev = {
	.name		= "dm3xx_ipipeif",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(dm365_ipipeif_resources),
	.resource	= dm365_ipipeif_resources,
	.dev = {
		.dma_mask		= &dm365_ipipeif_dma_mask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
		/* For IPIPEIF device type. 1 - DM365 */
		.platform_data		= (void *)1,
	},
};

static u64 dm365_osd_dma_mask = DMA_BIT_MASK(32);

static struct davinci_osd_platform_data dm365_osd_pdata = {
	.invert_field = false,
};

static struct resource dm365_osd_resources[] = {
	{
		.start          = IRQ_VENCINT,
		.end            = IRQ_VENCINT,
		.flags          = IORESOURCE_IRQ,
	},
	{
		.start          = DM365_OSD_REG_BASE,
		.end            = DM365_OSD_REG_BASE + OSD_REG_SIZE,
		.flags          = IORESOURCE_MEM,
	},
};

static struct platform_device dm365_osd_dev = {
	.name		= "davinci_osd",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(dm365_osd_resources),
	.resource	= dm365_osd_resources,
	.dev = {
		.dma_mask		= &dm365_osd_dma_mask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
		.platform_data		= &dm365_osd_pdata,
	},
};

static u64 dm365_venc_dma_mask = DMA_BIT_MASK(32);


static struct davinci_venc_platform_data dm365_venc_pdata = {
	.invert_field = false,
};
static struct resource dm365_venc_resources[] = {
	{
		.start          = DM365_VENC_REG_BASE,
		.end            = DM365_VENC_REG_BASE + 0x180,
		.flags          = IORESOURCE_MEM,
	},
};
static struct platform_device dm365_venc_dev = {
	.name		= "davinci_venc",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(dm365_venc_resources),
	.resource	= dm365_venc_resources,
	.dev = {
		.dma_mask		= &dm365_venc_dma_mask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
		.platform_data		= &dm365_venc_pdata,
	},
};

static u8 dm365_default_priorities[DAVINCI_N_AINTC_IRQ] = {
	[IRQ_VDINT0]			= 2,
	[IRQ_VDINT1]			= 6,
	[IRQ_VDINT2]			= 6,
	[IRQ_HISTINT]			= 6,
	[IRQ_H3AINT]			= 6,
	[IRQ_PRVUINT]			= 6,
	[IRQ_RSZINT]			= 6,
	[IRQ_DM365_INSFINT]		= 7,
	[IRQ_VENCINT]			= 6,
	[IRQ_ASQINT]			= 6,
	[IRQ_IMXINT]			= 6,
	[IRQ_DM365_IMCOPINT]		= 4,
	[IRQ_USBINT]			= 4,
	[IRQ_DM365_RTOINT]		= 7,
	[IRQ_DM365_TINT5]		= 7,
//	[IRQ_DM365_TINT6]		= 5,
	[IRQ_DM365_TINT6]		= 0,
	[IRQ_CCINT0]			= 5,
	[IRQ_CCERRINT]			= 5,
	[IRQ_TCERRINT0]			= 5,
	[IRQ_TCERRINT]			= 7,
	[IRQ_PSCIN]			= 4,
	[IRQ_DM365_SPINT2_1]		= 7,
	[IRQ_DM365_TINT7]		= 7,
	[IRQ_DM365_SDIOINT0]		= 7,
	[IRQ_MBXINT]			= 7,
	[IRQ_MBRINT]			= 7,
	[IRQ_MMCINT]			= 7,
	[IRQ_DM365_MMCINT1]		= 7,
	[IRQ_DM365_PWMINT3]		= 7,
	[IRQ_AEMIFINT]			= 2,
	[IRQ_DM365_SDIOINT1]		= 2,
	[IRQ_TINT0_TINT12]		= 7,
	[IRQ_TINT0_TINT34]		= 7,
	[IRQ_TINT1_TINT12]		= 7,
	[IRQ_TINT1_TINT34]		= 7,
	[IRQ_PWMINT0]			= 7,
	[IRQ_PWMINT1]			= 3,
	[IRQ_PWMINT2]			= 3,
	[IRQ_I2C]			= 3,
	[IRQ_UARTINT0]			= 3,
	[IRQ_UARTINT1]			= 3,
	[IRQ_DM365_RTCINT]		= 3,
	[IRQ_DM365_SPIINT0_0]		= 3,
	[IRQ_DM365_SPIINT3_0]		= 3,
	[IRQ_DM365_GPIO0]		= 3,
	[IRQ_DM365_GPIO1]		= 7,
	[IRQ_DM365_GPIO2]		= 4,
	[IRQ_DM365_GPIO3]		= 4,
	[IRQ_DM365_GPIO4]		= 7,
	[IRQ_DM365_GPIO5]		= 7,
	[IRQ_DM365_GPIO6]		= 7,
	[IRQ_DM365_GPIO7]		= 7,
	[IRQ_DM365_EMAC_RXTHRESH]	= 7,
	[IRQ_DM365_EMAC_RXPULSE]	= 7,
	[IRQ_DM365_EMAC_TXPULSE]	= 7,
	[IRQ_DM365_EMAC_MISCPULSE]	= 7,
	[IRQ_DM365_GPIO12]		= 7,
	[IRQ_DM365_GPIO13]		= 7,
	[IRQ_DM365_GPIO14]		= 7,
	[IRQ_DM365_GPIO15]		= 7,
	[IRQ_DM365_KEYINT]		= 7,
	[IRQ_DM365_TCERRINT2]		= 7,
	[IRQ_DM365_TCERRINT3]		= 7,
	[IRQ_DM365_EMUINT]		= 7,
};

/* Four Transfer Controllers on DM365 */
static const s8
dm365_queue_tc_mapping[][2] = {
	/* {event queue no, TC no} */
	{0, 0},
	{1, 1},
	{2, 2},
	{3, 3},
	{-1, -1},
};

static /*const*/ s8
dm365_queue_priority_mapping[][2] = {
	/* {event queue no, Priority} */
	{0, 7},
	{1, 7},
	{2, 7},
	{3, 0},
	{-1, -1},
};

static struct edma_soc_info edma_cc0_info = {
	.n_channel		= 64,
	.n_region		= 4,
	.n_slot			= 256,
	.n_tc			= 4,
	.n_cc			= 1,
	.queue_tc_mapping	= dm365_queue_tc_mapping,
	.queue_priority_mapping	= dm365_queue_priority_mapping,
	.default_queue		= EVENTQ_3,
};

static struct edma_soc_info *dm365_edma_info[EDMA_MAX_CC] = {
	&edma_cc0_info,
};

static struct resource edma_resources[] = {
	{
		.name	= "edma_cc0",
		.start	= 0x01c00000,
		.end	= 0x01c00000 + SZ_64K - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.name	= "edma_tc0",
		.start	= 0x01c10000,
		.end	= 0x01c10000 + SZ_1K - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.name	= "edma_tc1",
		.start	= 0x01c10400,
		.end	= 0x01c10400 + SZ_1K - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.name	= "edma_tc2",
		.start	= 0x01c10800,
		.end	= 0x01c10800 + SZ_1K - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.name	= "edma_tc3",
		.start	= 0x01c10c00,
		.end	= 0x01c10c00 + SZ_1K - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.name	= "edma0",
		.start	= IRQ_CCINT0,
		.flags	= IORESOURCE_IRQ,
	},
	{
		.name	= "edma0_err",
		.start	= IRQ_CCERRINT,
		.flags	= IORESOURCE_IRQ,
	},
	/* not using TC*_ERR */
};

static struct platform_device dm365_edma_device = {
	.name			= "edma",
	.id			= 0,
	.dev.platform_data	= dm365_edma_info,
	.num_resources		= ARRAY_SIZE(edma_resources),
	.resource		= edma_resources,
};

static struct resource dm365_asp_resources[] = {
	{
		.start	= DAVINCI_DM365_ASP0_BASE,
		.end	= DAVINCI_DM365_ASP0_BASE + SZ_8K - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= DAVINCI_DMA_ASP0_TX,
		.end	= DAVINCI_DMA_ASP0_TX,
		.flags	= IORESOURCE_DMA,
	},
	{
		.start	= DAVINCI_DMA_ASP0_RX,
		.end	= DAVINCI_DMA_ASP0_RX,
		.flags	= IORESOURCE_DMA,
	},
};

static struct platform_device dm365_asp_device = {
	.name		= "davinci-mcbsp",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(dm365_asp_resources),
	.resource	= dm365_asp_resources,
};

static struct resource dm365_vc_resources[] = {
	{
		.start	= DAVINCI_DM365_VC_BASE,
		.end	= DAVINCI_DM365_VC_BASE + SZ_1K - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= DAVINCI_DMA_VC_TX,
		.end	= DAVINCI_DMA_VC_TX,
		.flags	= IORESOURCE_DMA,
	},
	{
		.start	= DAVINCI_DMA_VC_RX,
		.end	= DAVINCI_DMA_VC_RX,
		.flags	= IORESOURCE_DMA,
	},
};

static struct platform_device dm365_vc_device = {
	.name		= "davinci_voicecodec",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(dm365_vc_resources),
	.resource	= dm365_vc_resources,
};

static struct resource dm365_rtc_resources[] = {
	{
		.start = DM365_RTC_BASE,
		.end = DM365_RTC_BASE + SZ_1K - 1,
		.flags = IORESOURCE_MEM,
	},
	{
		.start = IRQ_DM365_RTCINT,
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device dm365_rtc_device = {
	.name = "rtc_davinci",
	.id = 0,
	.num_resources = ARRAY_SIZE(dm365_rtc_resources),
	.resource = dm365_rtc_resources,
};

static struct map_desc dm365_io_desc[] = {
	{
		.virtual	= IO_VIRT,
		.pfn		= __phys_to_pfn(IO_PHYS),
		.length		= IO_SIZE,
		.type		= MT_DEVICE
	},
	{
		.virtual	= SRAM_VIRT,
		.pfn		= __phys_to_pfn(0x00010000),
		.length		= SZ_32K,
		.type		= MT_MEMORY_NONCACHED,
	},
};

static struct resource dm365_ks_resources[] = {
	{
		/* registers */
		.start = DM365_KEYSCAN_BASE,
		.end = DM365_KEYSCAN_BASE + SZ_1K - 1,
		.flags = IORESOURCE_MEM,
	},
	{
		/* interrupt */
		.start = IRQ_DM365_KEYINT,
		.end = IRQ_DM365_KEYINT,
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device dm365_ks_device = {
	.name		= "davinci_keyscan",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(dm365_ks_resources),
	.resource	= dm365_ks_resources,
};

/* Contents of JTAG ID register used to identify exact cpu type */
static struct davinci_id dm365_ids[] = {
	{
		.variant	= 0x0,
		.part_no	= 0xb83e,
		.manufacturer	= 0x017,
		.cpu_id		= DAVINCI_CPU_ID_DM365,
		.name		= "dm365_rev1.1",
	},
	{
		.variant	= 0x8,
		.part_no	= 0xb83e,
		.manufacturer	= 0x017,
		.cpu_id		= DAVINCI_CPU_ID_DM365,
		.name		= "dm36x_rev1.2",
	},
};

static u32 dm365_psc_bases[] = { DAVINCI_PWR_SLEEP_CNTRL_BASE };

static struct davinci_timer_info dm365_timer_info = {
	.timers		= davinci_timer_instance,
	.clockevent_id	= T0_BOT,
	.clocksource_id	= T0_TOP,
};

#define DM365_UART1_BASE	(IO_PHYS + 0x106000)

static struct plat_serial8250_port dm365_serial_platform_data[] = {
	{
		.mapbase	= DAVINCI_UART0_BASE,
		.irq		= IRQ_UARTINT0,
		.flags		= UPF_BOOT_AUTOCONF | UPF_SKIP_TEST |
				  UPF_IOREMAP,
		.iotype		= UPIO_MEM,
		.regshift	= 2,
		.uartclk	= 24000000,
	},
	{
		.mapbase	= DM365_UART1_BASE,
		.irq		= IRQ_UARTINT1,
		.flags		= UPF_BOOT_AUTOCONF | UPF_SKIP_TEST |
				  UPF_IOREMAP,
		.iotype		= UPIO_MEM,
		.regshift	= 2,
		.uartclk	= 86250000,
	},
	{
		.flags		= 0
	},
};

static struct platform_device dm365_serial_device = {
	.name			= "serial8250",
	.id			= PLAT8250_DEV_PLATFORM,
	.dev			= {
		.platform_data	= dm365_serial_platform_data,
	},
};

static struct davinci_soc_info davinci_soc_info_dm365 = {
	.io_desc		= dm365_io_desc,
	.io_desc_num		= ARRAY_SIZE(dm365_io_desc),
	.jtag_id_reg		= 0x01c40028,
	.ids			= dm365_ids,
	.ids_num		= ARRAY_SIZE(dm365_ids),
	.cpu_clks		= dm365_clks,
	.psc_bases		= dm365_psc_bases,
	.psc_bases_num		= ARRAY_SIZE(dm365_psc_bases),
	.pinmux_base		= DAVINCI_SYSTEM_MODULE_BASE,
	.pinmux_pins		= dm365_pins,
	.pinmux_pins_num	= ARRAY_SIZE(dm365_pins),
	.intc_base		= DAVINCI_ARM_INTC_BASE,
	.intc_type		= DAVINCI_INTC_TYPE_AINTC,
	.intc_irq_prios		= dm365_default_priorities,
	.intc_irq_num		= DAVINCI_N_AINTC_IRQ,
	.timer_info		= &dm365_timer_info,
	.gpio_type		= GPIO_TYPE_DAVINCI,
	.gpio_base		= DAVINCI_GPIO_BASE,
	.gpio_num		= 104,
	.gpio_irq		= IRQ_DM365_GPIO0,
	.gpio_unbanked		= 8,	/* really 16 ... skip muxed GPIOs */
	.serial_dev		= &dm365_serial_device,
	.emac_pdata		= &dm365_emac_pdata,
	.sram_dma		= 0x00010000,
	.sram_len		= SZ_32K,
	//.reset_device		= &davinci_wdt_device,
};

void __init dm365_init_asp(struct snd_platform_data *pdata)
{
	davinci_cfg_reg(DM365_MCBSP0_BDX);
	davinci_cfg_reg(DM365_MCBSP0_X);
	davinci_cfg_reg(DM365_MCBSP0_BFSX);
	davinci_cfg_reg(DM365_MCBSP0_BDR);
	davinci_cfg_reg(DM365_MCBSP0_R);
	davinci_cfg_reg(DM365_MCBSP0_BFSR);
	davinci_cfg_reg(DM365_EVT2_ASP_TX);
	davinci_cfg_reg(DM365_EVT3_ASP_RX);
	dm365_asp_device.dev.platform_data = pdata;
	platform_device_register(&dm365_asp_device);
}

 void ShowClocksInfo(void) {
 	static void __iomem *SystemRegisters; //access to system registers via memory remap
 	u32 regval; //Register value
 	char str[40]; //short strings container
 	char strEn[3]; //string "en" container
 	char strDis[4]; //string "dis" container
 	char vcCfg;
 
 	u32 mCLOCKOUT0EN;
 	u32 mCLOCKOUT1EN;
 	u32 mCLOCKOUT2EN;
 	u32 mDIV1;
 	u32 mDIV2;
 	u32 mDIV3;
 	u32 mHDVICPCLKS;
 	u32 mDDRCLKS;
 	u32 mKEYSCLKS;
 	u32 mARMCLKS;
 	u32 mPRTCCLKS;
 
 	u32 mIsPLL1en;
 	u32 mPLL1PreDiv;
 	u32 mPLL1Mul;
 	u32 mPLL1PostDiv;
 	u32 mIsPLL1PostDiv;
 	u32 mPLL1Clk;
 	u32 mPLL1DIV1;
 	u32 mPLL1DIV1en;
 	u32 mPLL1DIV2;
 	u32 mPLL1DIV2en;
 	u32 mPLL1DIV3;
 	u32 mPLL1DIV3en;
 	u32 mPLL1DIV4;
 	u32 mPLL1DIV4en;
 	u32 mPLL1DIV5;
 	u32 mPLL1DIV5en;
 	u32 mPLL1DIV6;
 	u32 mPLL1DIV6en;
 	u32 mPLL1DIV7;
 	u32 mPLL1DIV7en;
 	u32 mPLL1DIV8;
 	u32 mPLL1DIV8en;
 	u32 mPLL1DIV9;
 	u32 mPLL1DIV9en;
 
 	u32 mIsPLL2en;
 	u32 mPLL2PreDiv;
 	u32 mPLL2Mul;
 	u32 mPLL2PostDiv;
 	u32 mIsPLL2PostDiv;
 	u32 mPLL2Clk;
 	u32 mPLL2DIV1;
 	u32 mPLL2DIV1en;
 	u32 mPLL2DIV2;
 	u32 mPLL2DIV2en;
 	u32 mPLL2DIV3;
 	u32 mPLL2DIV3en;
 	u32 mPLL2DIV4;
 	u32 mPLL2DIV4en;
 	u32 mPLL2DIV5;
 	u32 mPLL2DIV5en;
 
 	u32 mARMCORECLK;
 	u32 mHDVICPCORECLK;
 	u32 mDDRCORECLK;
 
 	u32 mVoiceCodecCLK;
 
 
 #ifndef CONFIG_SND_DM365_SHOWFREQ //if undefined
 //This is applicable only if VoiceCodec selected in Linux Config
 //if no (for example, if AIC codec used) exit now.
 	return;
 #endif
 
 	strcpy(str,"unknown");
 	vcCfg=0;
 
 #ifdef CONFIG_SND_DM365_VOICE_CODEC_8KHZ
 	vcCfg=1;
 	strcpy(str,"8 kHz");
 #endif
 
 #ifdef CONFIG_SND_DM365_VOICE_CODEC_16KHZ
 	vcCfg=2;
 	strcpy(str,"16 kHz");
 #endif
 
 	printk("*** VOICECODEC CONFIGURATION is %s\n",str);
 
 	if(vcCfg==0) return;
 
         SystemRegisters=ioremap(DAVINCI_SYSTEM_MODULE_BASE,SECTION_SIZE); //try remap
         if(!SystemRegisters) {//if bad
 	  release_mem_region(DAVINCI_SYSTEM_MODULE_BASE,SECTION_SIZE);
 	  pr_err("ERROR: can't map DAVINCI_SYSTEM_MODULE_BASE\n");
 	  return; //EXIT
         }//if bad
 
 	//else remap is good
 
 	if(vcCfg==1) {//if Fs=8kHz
 // Fs ~= 8000 Hz  ; Actual Fs = 594000000/29/10/256=8001 ; Err=0.013%, 125ppm
 	  __raw_writel(0x801C,SystemRegisters+0xC00+0x160); //PLL2DIV4=28(29)
           __raw_writel(0x243f04fc,SystemRegisters+0x48);    //div2=9(10)
 	}//if Fs=8kHz
 
 	if(vcCfg==2) {//if Fs=16kHz
 // Fs ~= 16000 Hz ; Actual Fs = 594000000/29/5/256=16002 ; Err=0.013%, 125ppm
 	  __raw_writel(0x801C,SystemRegisters+0xC00+0x160); //PLL2DIV4=28(29)
           __raw_writel(0x243f027c,SystemRegisters+0x48);    //div2=4(5)
 	}//if Fs=16kHz
 
 // WARNING!!! 'Fs' is derived from PLL2 Clock. 
 // It is assumed that the PLL2 frequency is equal to 594000000 Hz in your system.
 // If not, you need to change PLL2DIV4 and div2 (see above).
 // See "TMS320DM36x Digital Media System-on-Chip (DMSoC) ARM Subsystem User's Guide"
 // chapter 5 "Device Clocking" and chapter 6 "PLL Controllers (PLLCs)"
 
 
 
 	strEn[0]='e'; 	strEn[1]='n'; 	strEn[2]=0; 			//"en"
 	strDis[0]='d'; 	strDis[1]='i'; 	strDis[2]='s';	strDis[3]=0;	//"dis"
 
         printk("*** Board Clocks:\n");
 
         regval= __raw_readl(SystemRegisters+0x48); //Read PERI_CLKCTL
         printk("* PERI_CLKCTL=0x%08X\n",regval);
 
 	mCLOCKOUT0EN 	= (regval&0x00000001)>>0;
 	mCLOCKOUT1EN 	= (regval&0x00000002)>>1;
 	mCLOCKOUT2EN 	= (regval&0x00000004)>>2;
 	mDIV1        	= (regval&0x00000078)>>3;
 	mDIV2        	= (regval&0x0000FF80)>>7;
 	mDIV3        	= (regval&0x03FF0000)>>16;
 	mHDVICPCLKS  	= (regval&0x04000000)>>26;
 	mDDRCLKS     	= (regval&0x08000000)>>27;
 	mKEYSCLKS    	= (regval&0x10000000)>>28;
 	mARMCLKS     	= (regval&0x20000000)>>29;
 	mPRTCCLKS    	= (regval&0x40000000)>>30;
 
         strcpy(str,strEn); if(mCLOCKOUT0EN!=0) strcpy(str,strDis);
         printk("* CLOCKOUT0EN is %2d = %s\n",mCLOCKOUT0EN,str);
 
         strcpy(str,strEn); if(mCLOCKOUT1EN!=0) strcpy(str,strDis);
         printk("* CLOCKOUT1EN is %2d = %s\n",mCLOCKOUT1EN,str);
 
         strcpy(str,strEn); if(mCLOCKOUT2EN!=0) strcpy(str,strDis);
         printk("* CLOCKOUT2EN is %2d = %s\n",mCLOCKOUT2EN,str);
 
         printk("* DIV1        is %2d = %d\n",mDIV1,mDIV1+1);
 
         printk("* DIV2        is %2d = %d\n",mDIV2,mDIV2+1);
 
         printk("* DIV3        is %2d = %d\n",mDIV3,mDIV3+1);
 
         strcpy(str,"PLLC1SYSCLK2"); if(mHDVICPCLKS!=0) strcpy(str,"PLLC2SYSCLK2");
         printk("* HDVICPCLKS  is %2d = %s\n",mHDVICPCLKS,str);
 
         strcpy(str,"PLLC1SYSCLK7"); if(mDDRCLKS!=0) strcpy(str,"PLLC2SYSCLK3");
         printk("* DDRCLKS     is %2d = %s\n",mDDRCLKS,str);
 
         strcpy(str,"RTCXI (MXI)"); if(mKEYSCLKS!=0) strcpy(str,"PLLC1AUXCLK");
         printk("* KEYSCLKS    is %2d = %s\n",mKEYSCLKS,str);
 
         strcpy(str,"PLLC1SYSCLK2"); if(mARMCLKS!=0) strcpy(str,"PLLC2SYSCLK2");
         printk("* ARMCLKS     is %2d = %s\n",mARMCLKS,str);
 
         strcpy(str,"RTCXI (OSC)"); if(mPRTCCLKS!=0) strcpy(str,"PLLC1AUXCLK");
         printk("* PRTCCLKS    is %2d = %s\n",mPRTCCLKS,str);
 
         printk("* CLKIN       = %d Hz\n",DM365_REF_FREQ);
 
         regval= __raw_readl(SystemRegisters+0x800+0x110); //Read PLL1CTL
 	mIsPLL1en		= (regval&0x00000001)>>0;
 
         regval= __raw_readl(SystemRegisters+0x800+0x114); //Read PLL1PreDiv
 	mPLL1PreDiv		= (regval&0x0000001F)>>0;
 
         printk("* PLL1PreDiv  is %2d = %2d\n",mPLL1PreDiv,mPLL1PreDiv+1);
 
         regval= __raw_readl(SystemRegisters+0x800+0x110); //Read PLL1Mul
 	mPLL1Mul		= (regval&0x000003FF)>>0;
         printk("* PLL1Mul     is %4d = %4d\n",mPLL1Mul,mPLL1Mul*2);
 
         regval= __raw_readl(SystemRegisters+0x800+0x128); //Read PLL1PostDiv
 	mPLL1PostDiv		= (regval&0x0000001F)>>0;
 	mIsPLL1PostDiv		= (regval&0x00008000)>>15;
 	strcpy(str,strDis); if(mIsPLL1PostDiv!=0) strcpy(str,strEn);
         printk("* PLL1PostDiv is %2d = %2d -- %s\n",mPLL1PostDiv,mPLL1PostDiv+1,str);
         if(mIsPLL1PostDiv==0) mPLL1PostDiv=0;
 
 	strcpy(str,"bypass"); if(mIsPLL1en!=0) strcpy(str,strEn);
 	mPLL1Clk = ( (DM365_REF_FREQ / (mPLL1PreDiv+1)) * (2*mPLL1Mul) ) / (mPLL1PostDiv+1);
         if(mIsPLL1en==0) mPLL1Clk=DM365_REF_FREQ;
         printk("* PLL1 CLK    = %d Hz -- %s\n",mPLL1Clk,str);
 
         regval= __raw_readl(SystemRegisters+0x800+0x118); //Read PLL1DIV1
 	mPLL1DIV1		= (regval&0x0000001F)>>0;
 	mPLL1DIV1en		= (regval&0x00008000)>>15;
 	strcpy(str,strDis); if(mPLL1DIV1en!=0) strcpy(str,strEn);
         printk("* PLL1DIV1    is %2d = %2d -- %s\n",mPLL1DIV1,mPLL1DIV1+1,str);
 	if(mPLL1DIV1en==0) mPLL1DIV1=0;
 
         regval= __raw_readl(SystemRegisters+0x800+0x11C); //Read PLL1DIV2
 	mPLL1DIV2		= (regval&0x0000001F)>>0;
 	mPLL1DIV2en		= (regval&0x00008000)>>15;
 	strcpy(str,strDis); if(mPLL1DIV2en!=0) strcpy(str,strEn);
         printk("* PLL1DIV2    is %2d = %2d -- %s\n",mPLL1DIV2,mPLL1DIV2+1,str);
 	if(mPLL1DIV2en==0) mPLL1DIV2=0;
 
         regval= __raw_readl(SystemRegisters+0x800+0x120); //Read PLL1DIV3
 	mPLL1DIV3		= (regval&0x0000001F)>>0;
 	mPLL1DIV3en		= (regval&0x00008000)>>15;
 	strcpy(str,strDis); if(mPLL1DIV3en!=0) strcpy(str,strEn);
         printk("* PLL1DIV3    is %2d = %2d -- %s\n",mPLL1DIV3,mPLL1DIV3+1,str);
 	if(mPLL1DIV3en==0) mPLL1DIV3=0;
 
         regval= __raw_readl(SystemRegisters+0x800+0x160); //Read PLL1DIV4
 	mPLL1DIV4		= (regval&0x0000001F)>>0;
 	mPLL1DIV4en		= (regval&0x00008000)>>15;
 	strcpy(str,strDis); if(mPLL1DIV4en!=0) strcpy(str,strEn);
         printk("* PLL1DIV4    is %2d = %2d -- %s\n",mPLL1DIV4,mPLL1DIV4+1,str);
 	if(mPLL1DIV4en==0) mPLL1DIV4=0;
 
         regval= __raw_readl(SystemRegisters+0x800+0x164); //Read PLL1DIV5
 	mPLL1DIV5		= (regval&0x0000001F)>>0;
 	mPLL1DIV5en		= (regval&0x00008000)>>15;
 	strcpy(str,strDis); if(mPLL1DIV5en!=0) strcpy(str,strEn);
 	printk("* PLL1DIV5    is %2d = %2d -- %s\n",mPLL1DIV5,mPLL1DIV5+1,str);
 	if(mPLL1DIV5en==0) mPLL1DIV5=0;
 
         regval= __raw_readl(SystemRegisters+0x800+0x168); //Read PLL1DIV6
 	mPLL1DIV6		= (regval&0x0000001F)>>0;
 	mPLL1DIV6en	= (regval&0x00008000)>>15;
 	strcpy(str,strDis); if(mPLL1DIV6en!=0) strcpy(str,strEn);
         printk("* PLL1DIV6    is %2d = %2d -- %s\n",mPLL1DIV6,mPLL1DIV6+1,str);
 	if(mPLL1DIV6en==0) mPLL1DIV6=0;
 
         regval= __raw_readl(SystemRegisters+0x800+0x16C); //Read PLL1DIV7
 	mPLL1DIV7		= (regval&0x0000001F)>>0;
 	mPLL1DIV7en	= (regval&0x00008000)>>15;
 	strcpy(str,strDis); if(mPLL1DIV7en!=0) strcpy(str,strEn);
         printk("* PLL1DIV7    is %2d = %2d -- %s\n",mPLL1DIV7,mPLL1DIV7+1,str);
 	if(mPLL1DIV7en==0) mPLL1DIV7=0;
 
         regval= __raw_readl(SystemRegisters+0x800+0x170); //Read PLL1DIV8
 	mPLL1DIV8		= (regval&0x0000001F)>>0;
 	mPLL1DIV8en	= (regval&0x00008000)>>15;
 	strcpy(str,strDis); if(mPLL1DIV8en!=0) strcpy(str,strEn);
         printk("* PLL1DIV8    is %2d = %2d -- %s\n",mPLL1DIV8,mPLL1DIV8+1,str);
 	if(mPLL1DIV8en==0) mPLL1DIV8=0;
 
         regval= __raw_readl(SystemRegisters+0x800+0x174); //Read PLL1DIV9
 	mPLL1DIV9		= (regval&0x0000001F)>>0;
 	mPLL1DIV9en	= (regval&0x00008000)>>15;
 	strcpy(str,strDis); if(mPLL1DIV9en!=0) strcpy(str,strEn);
         printk("* PLL1DIV9    is %2d = %2d -- %s\n",mPLL1DIV9,mPLL1DIV9+1,str);
 	if(mPLL1DIV9en==0) mPLL1DIV9=0;
 
         regval= __raw_readl(SystemRegisters+0xC00+0x110); //Read PLL2CTL
 	mIsPLL2en		= (regval&0x00000001)>>0;
 
         regval= __raw_readl(SystemRegisters+0xC00+0x114); //Read PLL2PreDiv
 	mPLL2PreDiv		= (regval&0x0000001F)>>0;
         printk("* PLL2PreDiv  is %2d = %2d\n",mPLL2PreDiv,mPLL2PreDiv+1);
 
         regval= __raw_readl(SystemRegisters+0xC00+0x110); //Read PLL2Mul
 	mPLL2Mul		= (regval&0x000003FF)>>0;
         printk("* PLL2Mul     is %4d = %4d\n",mPLL2Mul,mPLL2Mul*2);
 
         regval= __raw_readl(SystemRegisters+0xC00+0x128); //Read PLL2PostDiv
 	mPLL2PostDiv		= (regval&0x0000001F)>>0;
 	mIsPLL2PostDiv		= (regval&0x00008000)>>15;
 	strcpy(str,strDis); if(mIsPLL2PostDiv!=0) strcpy(str,strEn);
         printk("* PLL2PostDiv is %2d = %2d -- %s\n",mPLL2PostDiv,mPLL2PostDiv+1,str);
 	if(mIsPLL2PostDiv==0) mPLL2PostDiv=0;
 
 	strcpy(str,"bypass"); if(mIsPLL2en!=0) strcpy(str,strEn);
 	mPLL2Clk = ( (DM365_REF_FREQ / (mPLL2PreDiv+1)) * (2*mPLL2Mul) ) / (mPLL2PostDiv+1);
 	if(mIsPLL2en==0) mPLL2Clk=DM365_REF_FREQ;
         printk("* PLL2 CLK    = %d Hz -- %s\n",mPLL2Clk,str);
 
         regval= __raw_readl(SystemRegisters+0xC00+0x118); //Read PLL2DIV1
 	mPLL2DIV1		= (regval&0x0000001F)>>0;
 	mPLL2DIV1en		= (regval&0x00008000)>>15;
 	strcpy(str,strDis); if(mPLL2DIV1en!=0) strcpy(str,strEn);
         printk("* PLL2DIV1    is %2d = %2d -- %s\n",mPLL2DIV1,mPLL2DIV1+1,str);
 	if(mPLL2DIV1en==0) mPLL2DIV1=0;
 
         regval= __raw_readl(SystemRegisters+0xC00+0x11C); //Read PLL2DIV2
 	mPLL2DIV2		= (regval&0x0000001F)>>0;
 	mPLL2DIV2en	= (regval&0x00008000)>>15;
 	strcpy(str,strDis); if(mPLL2DIV2en!=0) strcpy(str,strEn);
         printk("* PLL2DIV2    is %2d = %2d -- %s\n",mPLL2DIV2,mPLL2DIV2+1,str);
 	if(mPLL2DIV2en==0) mPLL2DIV2=0;
 
         regval= __raw_readl(SystemRegisters+0xC00+0x120); //Read PLL2DIV3
 	mPLL2DIV3		= (regval&0x0000001F)>>0;
 	mPLL2DIV3en	= (regval&0x00008000)>>15;
 	strcpy(str,strDis); if(mPLL2DIV3en!=0) strcpy(str,strEn);
         printk("* PLL2DIV3    is %2d = %2d -- %s\n",mPLL2DIV3,mPLL2DIV3+1,str);
 	if(mPLL2DIV3en==0) mPLL2DIV3=0;
 
         regval= __raw_readl(SystemRegisters+0xC00+0x160); //Read PLL2DIV4
 	mPLL2DIV4		= (regval&0x0000001F)>>0;
 	mPLL2DIV4en	= (regval&0x00008000)>>15;
 	strcpy(str,strDis); if(mPLL2DIV4en!=0) strcpy(str,strEn);
         printk("* PLL2DIV4    is %2d = %2d -- %s\n",mPLL2DIV4,mPLL2DIV4+1,str);
 	if(mPLL2DIV4en==0) mPLL2DIV4=0;
 
         regval= __raw_readl(SystemRegisters+0xC00+0x164); //Read PLL2DIV5
 	mPLL2DIV5		= (regval&0x0000001F)>>0;
 	mPLL2DIV5en	= (regval&0x00008000)>>15;
 	strcpy(str,strDis); if(mPLL2DIV5en!=0) strcpy(str,strEn);
         printk("* PLL2DIV5    is %2d = %2d -- %s\n",mPLL2DIV5,mPLL2DIV5+1,str);
 	if(mPLL2DIV5en==0) mPLL2DIV5=0;
 
 	//check ARM CLOCK source
 	if(mARMCLKS==0) {
         	mARMCORECLK = mPLL1Clk / (mPLL1DIV2+1);
         }
 	else {
         	mARMCORECLK = mPLL2Clk / (mPLL2DIV2+1);
 	}
 	printk("* ARM CORE CLK           = %d Hz\n",mARMCORECLK);
 
 
 	//check HDVICP CLOCK source
 	if(mHDVICPCLKS==0) {
         	mHDVICPCORECLK = mPLL1Clk / (mPLL1DIV2+1);
         }
 	else {
         	mHDVICPCORECLK = mPLL2Clk / (mPLL2DIV2+1);
 	}
 	printk("* HDVICP CLK             = %d Hz\n",mHDVICPCORECLK);
 
         //check DDR CLOCK source
 	if(mDDRCLKS==0) {
         	mDDRCORECLK = mPLL1Clk / (mPLL1DIV7+1);
         }
 	else {
         	mDDRCORECLK = mPLL2Clk / (mPLL2DIV3+1);
 	}
 	printk("* DDR CLK supply for PHY = %d Hz ; Real DDR clk = %d Hz\n",mDDRCORECLK,mDDRCORECLK/2);
 
 	//calc VoiceCodec Clock	
 	mVoiceCodecCLK=(mPLL2Clk/(mPLL2DIV4+1))/(mDIV2+1);
 	printk("* VoiceCodec CLK         = %d Hz ; Sample Freq = %d Hz\n",mVoiceCodecCLK,mVoiceCodecCLK/256);
 
 	if(mPLL2Clk!=594000000) {
 		printk("*** WARNING!!! Your PPL2 clock is not 594Mhz. Check VoiceCodec sampling frequency (see above).\n");
 		printk("*** The VoiceCodec can work with Fs at range from 8kHz to 16kHz.\n");
 		printk("*** Maybe you need to change the divider in 'dm365.c' ('arch/arm/mach-davinci').\n");
 	}
 
 }


void __init dm365_init_vc(struct snd_platform_data *pdata)
{
	davinci_cfg_reg(DM365_EVT2_VC_TX);
	davinci_cfg_reg(DM365_EVT3_VC_RX);
	davinci_cfg_reg(DM365_INT_VCIF_ENABLE);
	dm365_vc_device.dev.platform_data = pdata;
	platform_device_register(&dm365_vc_device);
}

void __init dm365_init_ks(struct davinci_ks_platform_data *pdata)
{
	dm365_ks_device.dev.platform_data = pdata;
	platform_device_register(&dm365_ks_device);
}

void __init dm365_init_rtc(void)
{
	davinci_cfg_reg(DM365_INT_PRTCSS);
	platform_device_register(&dm365_rtc_device);
}

void __init dm365_init(void)
{
	davinci_common_init(&davinci_soc_info_dm365);
	ShowClocksInfo(); //Show Board Clocks
	davinci_map_sysmod();
}

#define DM365_ISP5_REG_BASE		0x01C70000

static struct resource dm365_vpss_resources[] = {
	{
		/* VPSS ISP5 Base address */
		.name           = "vpss",
		.start          = 0x01c70000,
		.end            = 0x01c70000 + 0xff,
		.flags          = IORESOURCE_MEM,
	},
	{
		/* VPSS CLK Base address */
		.name           = "vpss",
		.start          = 0x01c70200,
		.end            = 0x01c70200 + 0xff,
		.flags          = IORESOURCE_MEM,
	},
};

static struct platform_device dm365_vpss_device = {
	.name			= "vpss",
	.id			= -1,
	.dev.platform_data	= "dm365_vpss",
	.num_resources		= ARRAY_SIZE(dm365_vpss_resources),
	.resource		= dm365_vpss_resources,
};

static struct resource vpfe_resources[] = {
	{
		.start          = IRQ_VDINT0,
		.end            = IRQ_VDINT0,
		.flags          = IORESOURCE_IRQ,
	},
	{
		.start          = IRQ_VDINT1,
		.end            = IRQ_VDINT1,
		.flags          = IORESOURCE_IRQ,
	},
};

static u64 vpfe_capture_dma_mask = DMA_BIT_MASK(32);
static struct platform_device vpfe_capture_dev = {
	.name           = CAPTURE_DRV_NAME,
	.id             = -1,
	.num_resources  = ARRAY_SIZE(vpfe_resources),
	.resource       = vpfe_resources,
	.dev = {
		.dma_mask               = &vpfe_capture_dma_mask,
		.coherent_dma_mask      = DMA_BIT_MASK(32),
	},
};

static struct resource isif_resource[] = {
	/* ISIF Base address */
	{
		.start          = 0x01c71000,
		.end            = 0x01c71000 + 0x1ff,
		.flags          = IORESOURCE_MEM,
	},
	/* ISIF Linearization table 0 */
	{
		.start          = 0x1C7C000,
		.end            = 0x1C7C000 + 0x2ff,
		.flags          = IORESOURCE_MEM,
	},
	/* ISIF Linearization table 1 */
	{
		.start          = 0x1C7C400,
		.end            = 0x1C7C400 + 0x2ff,
		.flags          = IORESOURCE_MEM,
	},
};
static struct platform_device dm365_isif_dev = {
	.name           = "dm365_isif",
	.id             = -1,
	.num_resources  = ARRAY_SIZE(isif_resource),
	.resource       = isif_resource,
	.dev = {
		.dma_mask               = &vpfe_capture_dma_mask,
		.coherent_dma_mask      = DMA_BIT_MASK(32),
	},
};

static int __init dm365_init_devices(void)
{
	if (!cpu_is_davinci_dm365())
		return 0;

	davinci_cfg_reg(DM365_INT_EDMA_CC);
	platform_device_register(&dm365_edma_device);

	/*
	* setup Mux configuration for vpfe input and register
	* vpfe capture platform device
	*/
	platform_device_register(&dm365_vpss_device);
	platform_device_register(&dm365_ipipeif_dev);
	platform_device_register(&dm365_isif_dev);
	platform_device_register(&vpfe_capture_dev);

	/* Register OSD device */
	platform_device_register(&dm365_osd_dev);

	/* Register VENC device */
	platform_device_register(&dm365_venc_dev);

	platform_device_register(&dm365_mdio_device);
	platform_device_register(&dm365_emac_device);
	clk_add_alias(NULL, dev_name(&dm365_mdio_device.dev),
			NULL, &dm365_emac_device.dev);

	return 0;
}
postcore_initcall(dm365_init_devices);

void dm365_set_vpfe_config(struct vpfe_config *cfg)
{
       vpfe_capture_dev.dev.platform_data = cfg;
}
