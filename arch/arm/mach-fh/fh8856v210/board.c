#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/io.h>
#include <linux/mmc/dw_mmc.h>
#include <linux/clk.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/spi/spi.h>
#include <linux/spi/flash.h>
#include <linux/phy.h>
#include <linux/dma-mapping.h>
#include <linux/spi/eeprom.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/if_alg.h>
#include <linux/mmc/host.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <mach/pmu.h>
#include <mach/io.h>
#include <mach/board_config.h>
#include <mach/pinctrl.h>
#include <mach/clock.h>
#include <mach/fh_uart_plat.h>
#include <mach/fh_mci_plat.h>
#include <mach/fh_gmac_plat.h>
#include <mach/fh_gpio_plat.h>
#include <mach/fh_efuse_plat.h>
#include <mach/fh_dma_plat.h>
#include <mach/fh_spi_plat.h>
#include <mach/fh_i2s_plat.h>
#include <mach/fh_pwm_plat.h>
#include <mach/fh_wdt_plat.h>
#include <mach/fh_usb_plat.h>
#include <mach/fh_sadc_plat.h>
#include <mach/fh_rtc_plat.h>
#include "chip.h"

struct uart_port fh_serial_ports[FH_UART_NUMBER];

static struct map_desc fh8856v210_io_desc[] = {
	{
		.virtual	= VA_RAM_REG_BASE,
		.pfn		= __phys_to_pfn(RAM_BASE),
		.length		= SZ_16K,
		.type		= MT_MEMORY_RWX,
	},
	{
		.virtual	= VA_DDRC_REG_BASE,
		.pfn		= __phys_to_pfn(DDRC_REG_BASE),
		.length		= SZ_16K,
		.type		= MT_DEVICE,
	},
	{
		.virtual	= VA_INTC_REG_BASE,
		.pfn		= __phys_to_pfn(INTC_REG_BASE),
		.length		= SZ_16K,
		.type		= MT_DEVICE,
	},
	{
		.virtual	= VA_TIMER_REG_BASE,
		.pfn		= __phys_to_pfn(TIMER_REG_BASE),
		.length		= SZ_16K,
		.type		= MT_DEVICE,
	},
	{
		.virtual	= VA_PMU_REG_BASE,
		.pfn		= __phys_to_pfn(PMU_REG_BASE),
		.length		= SZ_16K,
		.type		= MT_DEVICE,
	},
	{
		.virtual	= VA_UART0_REG_BASE,
		.pfn		= __phys_to_pfn(UART0_REG_BASE),
		.length		= SZ_16K,
		.type		= MT_DEVICE,
	},
	{
		.virtual	= VA_UART1_REG_BASE,
		.pfn		= __phys_to_pfn(UART1_REG_BASE),
		.length		= SZ_16K,
		.type		= MT_DEVICE,
	},
	{
		.virtual	= VA_UART2_REG_BASE,
		.pfn		= __phys_to_pfn(UART2_REG_BASE),
		.length		= SZ_16K,
		.type		= MT_DEVICE,
	},

};

static struct resource fh_gpio0_resources[] = {
	{
		.start		= GPIO0_REG_BASE,
		.end		= GPIO0_REG_BASE + SZ_16K - 1,
		.flags		= IORESOURCE_MEM,
	},

	{
		.start		= GPIO0_IRQ,
		.end		= GPIO0_IRQ,
		.flags		= IORESOURCE_IRQ,
	},
};

static struct resource fh_gpio1_resources[] = {
	{
		.start		= GPIO1_REG_BASE,
		.end		= GPIO1_REG_BASE + SZ_16K - 1,
		.flags		= IORESOURCE_MEM,
	},

	{
		.start		= GPIO1_IRQ,
		.end		= GPIO1_IRQ,
		.flags		= IORESOURCE_IRQ,
	},
};

static struct resource fh_uart0_resources[] = {
	{
		.start		= (UART0_REG_BASE),
		.end		= (UART0_REG_BASE) + SZ_16K - 1,
		.flags		= IORESOURCE_MEM,
	},

	{
		.start		= UART0_IRQ,
		.end		= UART0_IRQ,
		.flags		= IORESOURCE_IRQ,
	},
};

static struct resource fh_uart1_resources[] = {
	{
		.start		= (UART1_REG_BASE),
		.end		= (UART1_REG_BASE) + SZ_16K - 1,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= UART1_IRQ,
		.end		= UART1_IRQ,
		.flags		= IORESOURCE_IRQ,
	},
};
static struct resource fh_uart2_resources[] = {
	{
		.start		= (UART2_REG_BASE),
		.end		= (UART2_REG_BASE) + SZ_16K - 1,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= UART2_IRQ,
		.end		= UART2_IRQ,
		.flags		= IORESOURCE_IRQ,
	}
};
static struct resource fh_sdc0_resources[] = {
	{
		.start		= SDC0_REG_BASE,
		.end		= SDC0_REG_BASE + SZ_16K - 1,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= SDC0_IRQ,
		.end		= SDC0_IRQ,
		.flags		= IORESOURCE_IRQ,
	},
};
static struct resource fh_sdc1_resources[] = {
	{
		.start		= SDC1_REG_BASE,
		.end		= SDC1_REG_BASE + SZ_16K - 1,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= SDC1_IRQ,
		.end		= SDC1_IRQ,
		.flags		= IORESOURCE_IRQ,
	},
};

static struct resource fh_gmac_resources[] = {
	{
		.start		= GMAC_REG_BASE,
		.end		= GMAC_REG_BASE + SZ_16K - 1,
		.flags		= IORESOURCE_MEM,
	},

	{
		.start		= GMAC_IRQ,
		.end		= GMAC_IRQ,
		.flags		= IORESOURCE_IRQ,
	}
};

static struct resource fh_wdt_resources[] = {
	{
		.start		= WDT_REG_BASE,
		.end		= WDT_REG_BASE + SZ_16K - 1,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= WDT_IRQ,
		.end		= WDT_IRQ,
		.flags		= IORESOURCE_IRQ,
	},
};

#ifdef CONFIG_FH_PERF_MON
static struct resource fh_perf_resources[] = {
		{
			.start	= PMU_REG_BASE,
			.end	= PMU_REG_BASE + SZ_16K - 1,
			.flags	= IORESOURCE_MEM,
		},
		{
			.start	= PERF_IRQ,
			.end	= PERF_IRQ,
			.flags	= IORESOURCE_IRQ,
		},
};
#endif


static struct fh_gmac_platform_data fh_gmac_data = {
	.phy_reset_pin = 29,
};

static struct fh_uart_dma uart1_dma_info = {
#ifdef CONFIG_UART_TX_DMA
	.tx_hs_no       = UART1_TX_HW_HANDSHAKE,
	.tx_dma_channel = UART1_DMA_TX_CHAN,
#endif
	.rx_hs_no       = UART1_RX_HW_HANDSHAKE,
	.rx_dma_channel = UART1_DMA_RX_CHAN,
	.rx_xmit_len    = 16,
};

static struct fh_uart_dma uart2_dma_info = {
#ifdef CONFIG_UART_TX_DMA
	.tx_hs_no       = UART2_TX_HW_HANDSHAKE,
	.tx_dma_channel = UART2_DMA_TX_CHAN,
#endif
	.rx_hs_no       = UART2_RX_HW_HANDSHAKE,
	.rx_dma_channel = UART2_DMA_RX_CHAN,
	.rx_xmit_len    = 16,
};


static struct fh_platform_uart fh_uart_platform_data[] = {
	{
		.mapbase	= UART0_REG_BASE,
		.fifo_size	= 16,
		.irq		= UART0_IRQ,
		.uartclk	= 16666667,
		.use_dma	= 0,
		.dma_info	= NULL,
	},
	{
		.mapbase	= UART1_REG_BASE,
		.fifo_size	= 32,
		.irq		= UART1_IRQ,
		.uartclk	= 16666667,
		.use_dma	= 0,
		.dma_info	= &uart1_dma_info,
	},
	{
		.mapbase	= UART2_REG_BASE,
		.fifo_size	= 32,
		.irq		= UART2_IRQ,
		.uartclk	= 16666667,
		.use_dma	= 0,
		.dma_info	= &uart2_dma_info,
	},
};

static struct resource fh_pwm_resources[] = {
	{
		.start		= PWM_REG_BASE,
		.end		= PWM_REG_BASE + SZ_16K - 1,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= PWM_IRQ,
		.end		= PWM_IRQ,
		.flags		= IORESOURCE_IRQ,
	},
};

static struct resource fh_i2c_resources_0[] = {
	{
		.start		= I2C0_REG_BASE,
		.end		= I2C0_REG_BASE + SZ_16K - 1,
		.flags		= IORESOURCE_MEM,
	},

	{
		.start		= I2C0_IRQ,
		.end		= I2C0_IRQ,
		.flags		= IORESOURCE_IRQ,
	},
};
static struct resource fh_i2c_resources_1[] = {
	{
		.start		= I2C1_REG_BASE,
		.end		= I2C1_REG_BASE + SZ_16K - 1,
		.flags		= IORESOURCE_MEM,
	},

	{
		.start		= I2C1_IRQ,
		.end		= I2C1_IRQ,
		.flags		= IORESOURCE_IRQ,
	},
};
static struct resource fh_i2c_resources_2[] = {
	{
		.start		= I2C2_REG_BASE,
		.end		= I2C2_REG_BASE + SZ_16K - 1,
		.flags		= IORESOURCE_MEM,
	},

	{
		.start		= I2C2_IRQ,
		.end		= I2C2_IRQ,
		.flags		= IORESOURCE_IRQ,
	},
};
static struct resource fh_rtc_resources[] = {
	{
		.start		= RTC_REG_BASE,
		.end		= RTC_REG_BASE + SZ_16K - 1,
		.flags		= IORESOURCE_MEM,
	},

	{
		.start		= RTC_IRQ,
		.end		= RTC_IRQ,
		.flags		= IORESOURCE_IRQ,
	},
};

static struct fh_gpio_chip fh_gpio0_chip = {
	.chip = {
		.owner = THIS_MODULE,
		.label = "FH_GPIO0",
		.base = 0,
		.ngpio = 32,
	},
};

static struct fh_gpio_chip fh_gpio1_chip = {
	.chip = {
		.owner = THIS_MODULE,
		.label = "FH_GPIO1",
		.base = 32,
		.ngpio = 32,
	},
};

static struct fh_pwm_data pwm_data = {
	.npwm = 12,
};

static struct resource fh_sadc_resources[] = {
	{
		.start		= SADC_REG_BASE,
		.end		= SADC_REG_BASE + SZ_16K - 1,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= SADC_IRQ,
		.end		= SADC_IRQ,
		.flags		= IORESOURCE_IRQ,
	},
};

static struct resource fh_aes_resources[] = {
	{
		.start		= AES_REG_BASE,
		.end		= AES_REG_BASE + SZ_16K - 1,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= AES_IRQ,
		.end		= AES_IRQ,
		.flags		= IORESOURCE_IRQ,
	},
};

static struct resource fh_efuse_resources[] = {
	{
		.start		= EFUSE_REG_BASE,
		.end		= EFUSE_REG_BASE + SZ_16K - 1,
		.flags		= IORESOURCE_MEM,
	},
};

#ifdef CONFIG_FH_DMAC
static struct resource fh_dma_resources[] = {
	{
		.start		= (DMAC_REG_BASE),
		.end		= (DMAC_REG_BASE) + SZ_16K - 1,
		.flags		= IORESOURCE_MEM,
	},

	{
		.start		= DMAC0_IRQ,
		.end		= DMAC0_IRQ,
		.flags		= IORESOURCE_IRQ,
	},
};
#endif

#ifdef CONFIG_FH_AXI_DMAC
static struct resource fh_axi_dma_resources[] = {
	{
		.start          = (DMAC_REG_BASE),
		.end            = (DMAC_REG_BASE) + SZ_16K - 1,
		.flags          = IORESOURCE_MEM,
	},
	{
		.start          = DMAC0_IRQ,
		.end            = DMAC0_IRQ,
		.flags          = IORESOURCE_IRQ,
	},
};
#endif

static struct resource fh_spi0_resources[] = {
	{
		.start		= SPI0_REG_BASE,
		.end		= SPI0_REG_BASE + SZ_16K - 1,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= SPI0_IRQ,
		.end		= SPI0_IRQ,
		.flags		= IORESOURCE_IRQ,
	},
};

static struct resource fh_spi1_resources[] = {
	{
		.start		= SPI1_REG_BASE,
		.end		= SPI1_REG_BASE + SZ_16K - 1,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= SPI1_IRQ,
		.end		= SPI1_IRQ,
		.flags		= IORESOURCE_IRQ,
	},
};

static struct resource fh_spi2_resources[] = {
	{
		.start		= SPI2_REG_BASE,
		.end		= SPI2_REG_BASE + SZ_16K - 1,
		.flags		= IORESOURCE_MEM,
		.name = "fh spi2 mem",
	},
	{
		.start		= SPI2_IRQ,
		.end		= SPI2_IRQ,
		.flags		= IORESOURCE_IRQ,
		.name = "fh spi2 irq",
	},
};

static struct resource fh_usb_resources[] = {
	{
		.start		= USBC_REG_BASE,
		.end		= USBC_REG_BASE + SZ_1M - 1,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= USBC_IRQ,
		.end		= USBC_IRQ,
		.flags		= IORESOURCE_IRQ,
	},
};
static unsigned int  fh_mci_sys_card_detect_fixed(struct fhmci_host *host)
{
	return 0;
}

struct fh_mci_board fh_mci = {
	.num_slots = 1,
	.get_cd = fh_mci_sys_card_detect_fixed,
	.bus_hz = 50000000,
	.detect_delay_ms = 200,
	.caps = MMC_CAP_4_BIT_DATA,
	/*8:180 degree*/
	.drv_degree = 8,
	.sam_degree = 0,
	.rescan_max_num = 2,
};

struct fh_mci_board fh_mci_sd = {
	.num_slots = 1,
	.bus_hz = 50000000,
	.detect_delay_ms = 200,
	.caps = MMC_CAP_4_BIT_DATA,
	/*8:180 degree*/
	.drv_degree = 8,
	.sam_degree = 0,
};

static struct platform_device fh_gmac_device = {
	.name           = "fh_gmac",
	.id             = 0,
	.num_resources  = ARRAY_SIZE(fh_gmac_resources),
	.resource       = fh_gmac_resources,
	.dev = {
		.coherent_dma_mask = DMA_BIT_MASK(32),
		.platform_data = &fh_gmac_data,
	},
};

struct platform_device fh_sd0_device = {
	.name			= "fh_mci",
	.id			= 0,
	.num_resources		= ARRAY_SIZE(fh_sdc0_resources),
	.resource		= fh_sdc0_resources,
	.dev			= {
	.coherent_dma_mask	= DMA_BIT_MASK(32),
	.platform_data = &fh_mci_sd,
	}
};

struct platform_device fh_sd1_device = {
	.name			= "fh_mci",
	.id			= 1,
	.num_resources		= ARRAY_SIZE(fh_sdc1_resources),
	.resource		= fh_sdc1_resources,
	.dev			= {
	.coherent_dma_mask	= DMA_BIT_MASK(32),
	.platform_data = &fh_mci,
	}
};

struct fh_sadc_platform_data fh_sadc_data = {
	.ref_vol = 1800,
	.active_bit = 0xfff,
};

static struct platform_device fh_sadc_device = {
	.name			= "fh_sadc",
	.id			= 0,
	.num_resources		= ARRAY_SIZE(fh_sadc_resources),
	.resource		= fh_sadc_resources,
	.dev			= {
	.platform_data = &fh_sadc_data,
	},
};

static struct platform_device fh_uart0_device = {
	.name				= "ttyS",
	.id			= 0,
	.num_resources		= ARRAY_SIZE(fh_uart0_resources),
	.resource		= fh_uart0_resources,
	.dev.platform_data	= &fh_uart_platform_data[0],
};

static struct platform_device fh_uart1_device = {
	.name				= "ttyS",
	.id					= 1,
	.num_resources		= ARRAY_SIZE(fh_uart1_resources),
	.resource			= fh_uart1_resources,
	.dev.platform_data	= &fh_uart_platform_data[1],
};

static struct platform_device fh_uart2_device = {
	.name				= "ttyS",
	.id					= 2,
	.num_resources		= ARRAY_SIZE(fh_uart2_resources),
	.resource			= fh_uart2_resources,
	.dev.platform_data	= &fh_uart_platform_data[2],
};

static struct platform_device fh_pinctrl_device = {
	.name			= "fh_pinctrl",
	.id			= 0,
};

static struct platform_device fh_i2c0_device = {
	.name			= "fh_i2c",
	.id			= 0,
	.num_resources		= ARRAY_SIZE(fh_i2c_resources_0),
	.resource		= fh_i2c_resources_0,
};

static struct platform_device fh_i2c1_device = {
	.name			= "fh_i2c",
	.id			= 1,
	.num_resources		= ARRAY_SIZE(fh_i2c_resources_1),
	.resource		= fh_i2c_resources_1,
};

static struct platform_device fh_i2c2_device = {
	.name			= "fh_i2c",
	.id			= 2,
	.num_resources		= ARRAY_SIZE(fh_i2c_resources_2),
	.resource		= fh_i2c_resources_2,
};

static struct fh_rtc_plat_data rtc_plat_data[] = {
	{
		.lut_cof = 58,
		.lut_offset = 0xff,
		.tsensor_cp_default_out = 0x993,
		.clk_name = "rtc_hclk_gate",
	},
	{
		.lut_cof = 71,
		.lut_offset = 0xf6,
		.tsensor_cp_default_out = 0x9cc,
		.clk_name = "rtc_hclk_gate",
	}
};

static struct platform_device fh_rtc_device = {
	.name			= "fh_rtc",
	.id			= 0,
	.num_resources		= ARRAY_SIZE(fh_rtc_resources),
	.resource		= fh_rtc_resources,
	.dev.platform_data	= &rtc_plat_data[0],
};

static struct resource fh_i2s_resources[] = {
	{
		.start		= I2S_REG_BASE,
		.end		= I2S_REG_BASE + SZ_16K - 1,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= ACW_REG_BASE,
		.end		= ACW_REG_BASE + SZ_16K - 1,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= I2S0_IRQ,
		.end		= I2S0_IRQ,
		.flags		= IORESOURCE_IRQ,
	},
};

static struct fh_i2s_platform_data fh_i2s_data = {
	.dma_capture_channel = 4,
	.dma_playback_channel = 5,
	.dma_master = 0,
	.dma_rx_hs_num = 10,
	.dma_tx_hs_num = 11,
	.clk = "i2s_clk",
	.acodec_mclk = "ac_clk",
};

static struct platform_device fh_i2s_device = {
	.name			= "fh_audio",
	.id			= 0,
	.num_resources		= ARRAY_SIZE(fh_i2s_resources),
	.resource		= fh_i2s_resources,
	.dev			= {
		.platform_data = &fh_i2s_data,
	},
};

static struct platform_device fh_gpio0_device = {
	.name			= GPIO_NAME,
	.id				= 0,
	.num_resources	= ARRAY_SIZE(fh_gpio0_resources),
	.resource		= fh_gpio0_resources,
	.dev			= {
		.platform_data = &fh_gpio0_chip,
	},
};

static struct platform_device fh_gpio1_device = {
	.name			= GPIO_NAME,
	.id				= 1,
	.num_resources	= ARRAY_SIZE(fh_gpio1_resources),
	.resource		= fh_gpio1_resources,
	.dev			= {
		.platform_data = &fh_gpio1_chip,
	},
};

static struct platform_device fh_aes_device = {
	.name			= "fh_aes",
	.id			= 0,
	.num_resources		= ARRAY_SIZE(fh_aes_resources),
	.resource		= fh_aes_resources,
	.dev			= {
		.platform_data = NULL,
	},
};

struct fh_efuse_platform_data fh_efuse_plat_data = {
	.efuse_support_flag = CRYPTO_CPU_SET_KEY |
	CRYPTO_EX_MEM_SET_KEY |
	CRYPTO_EX_MEM_SWITCH_KEY |
	CRYPTO_EX_MEM_4_ENTRY_1_KEY |
	CRYPTO_EX_MEM_INDEP_POWER,
};



#define FH_SPI0_CS0	(6)
#define FH_SPI0_CS1	(55)

#define FH_SPI1_CS0	(14)
#define FH_SPI1_CS1	(57)

#define SPI0_FIFO_DEPTH				(128)
#define SPI0_CLK_IN				(200000000)
#define SPI0_MAX_SLAVE_NO			(2)
#define SPI0_DMA_RX_CHANNEL			(0)
#define SPI0_DMA_TX_CHANNEL			(1)

#define SPI1_FIFO_DEPTH				(64)
#define SPI1_CLK_IN				(100000000)
#define SPI1_MAX_SLAVE_NO			(2)
#define SPI1_DMA_RX_CHANNEL			(2)
#define SPI1_DMA_TX_CHANNEL			(3)

#define SPI2_CLK_IN				(100000000)

/* SPI_TRANSFER_USE_DMA */
static struct fh_spi_platform_data fh_spi0_data = {
	.bus_no = 0,
	.apb_clock_in = SPI0_CLK_IN,
	.clock_source = {100000000, 150000000, 200000000},
	.clock_source_num = 3,
	.slave_max_num = SPI0_MAX_SLAVE_NO,
	.cs_data[0].GPIO_Pin = FH_SPI0_CS0,
	.cs_data[0].name = "spi0_cs0",
	.cs_data[1].GPIO_Pin = FH_SPI0_CS1,
	.cs_data[1].name = "spi0_cs1",
	.clk_name = "spi0_clk",
	.dma_transfer_enable = SPI_TRANSFER_USE_DMA,
	.rx_dma_channel = SPI0_DMA_RX_CHANNEL,
	.rx_handshake_num = 4,
	/*dma use inc mode could move data by burst mode...*/
	/*or move data use single mode with low efficient*/
	.ctl_wire_support = ONE_WIRE_SUPPORT | DUAL_WIRE_SUPPORT |
	MULTI_WIRE_SUPPORT,
};

static struct fh_spi_platform_data fh_spi1_data = {
	.bus_no = 1,
	.apb_clock_in = SPI1_CLK_IN,
	.clock_source = {SPI1_CLK_IN},
	.clock_source_num = 1,
	.slave_max_num = SPI1_MAX_SLAVE_NO,
	.cs_data[0].GPIO_Pin = FH_SPI1_CS0,
	.cs_data[0].name = "spi1_cs0",
	.cs_data[1].GPIO_Pin = FH_SPI1_CS1,
	.cs_data[1].name = "spi1_cs1",
	.clk_name = "spi1_clk",
	.ctl_wire_support = 0,
};

static struct fh_spi_platform_data fh_spi2_data = {
	.apb_clock_in = SPI2_CLK_IN,
	.dma_transfer_enable = 0,
	.rx_handshake_num = 12,
	.clk_name = "spi2_clk",
	.ctl_wire_support = 0,
};

static struct platform_device fh_efuse_device = {
	.name			= "fh_efuse",
	.id			= 0,
	.num_resources		= ARRAY_SIZE(fh_efuse_resources),
	.resource		= fh_efuse_resources,
	.dev			= {
		.platform_data = &fh_efuse_plat_data,
	},
};

#ifdef CONFIG_FH_DMAC
static struct fh_dma_platform_data fh_dma_data = {
	.chan_priority  = CHAN_PRIORITY_ASCENDING,
	.nr_channels    = 6,
	.clk_name	= "ahb_clk",
};

static struct platform_device fh_dma_device = {
	.name			= "fh_dmac",
	.id			= 0,
	.num_resources		= ARRAY_SIZE(fh_dma_resources),
	.resource		= fh_dma_resources,
	.dev			= {
		.platform_data = &fh_dma_data,
	},
};
#endif

#ifdef CONFIG_FH_AXI_DMAC
struct fh_axi_dma_platform_data axi_dma_plat_data = {
	.chan_priority  = CHAN_PRIORITY_ASCENDING,
	.clk_name       = "ahb_clk",
};

static struct platform_device fh_axi_dma_device = {
	.name                   = "fh_axi_dmac",
	.id                     = 0,
	.num_resources          = ARRAY_SIZE(fh_axi_dma_resources),
	.resource               = fh_axi_dma_resources,
	.dev                    = {
		.platform_data = &axi_dma_plat_data,
	},
};
#endif



static struct platform_device fh_spi0_device = {
	.name			= "fh_spi",
	.id			= 0,
	.num_resources		= ARRAY_SIZE(fh_spi0_resources),
	.resource		= fh_spi0_resources,
	.dev			= {
		.platform_data = &fh_spi0_data,
	},
};

static struct platform_device fh_spi1_device = {
	.name			= "fh_spi",
	.id			= 1,
	.num_resources		= ARRAY_SIZE(fh_spi1_resources),
	.resource		= fh_spi1_resources,
	.dev			= {
		.platform_data = &fh_spi1_data,
	},
};

static struct platform_device fh_spi2_device = {
	.name			= "fh_spi_slave",
	.id			= 0,
	.num_resources		= ARRAY_SIZE(fh_spi2_resources),
	.resource		= fh_spi2_resources,
	.dev			= {
		.platform_data = &fh_spi2_data,
	},
};

#ifdef CONFIG_FH_PERF_MON
static struct platform_device fh_perf_device = {
	.name	= "fh_perf_mon",
	.id	= 0,
	.num_resources	= ARRAY_SIZE(fh_perf_resources),
	.resource	= fh_perf_resources,
	.dev	= {
		.platform_data = NULL,
	},
};
#endif

static struct fh_wdt_platform_data fh_wdt_data = {
	.mode   = MODE_DISCRETE,
};

struct platform_device fh_wdt_device = {
	.name			= "fh_wdt",
	.id				= 0,
	.num_resources	= ARRAY_SIZE(fh_wdt_resources),
	.resource		= fh_wdt_resources,
	.dev			= {
		.platform_data = &fh_wdt_data,
	}
};

static struct platform_device fh_pwm_device = {
	.name			= "fh_pwm",
	.id				= 0,
	.num_resources	= ARRAY_SIZE(fh_pwm_resources),
	.resource		= fh_pwm_resources,
	.dev			= {
		.platform_data = &pwm_data,
	},
};

static struct fh_usb_platform_data fh_usb_data = {
	.dr_mode  = "host",
	.vbus_pwren = 47,
};

struct platform_device fh_usb_device = {
	.name			= "fh_usb",
	.id				= 0,
	.num_resources	= ARRAY_SIZE(fh_usb_resources),
	.resource		= fh_usb_resources,
	.dev			= {
		.platform_data = &fh_usb_data,
	}
};

#ifdef CONFIG_FH_TSENSOR
struct platform_device fh_tsensor_device = {
	.name			= "fh_tsensor",
	.id				= 0,
};
#endif

static struct platform_device *fh8856v210_devices[] __initdata = {
	&fh_uart0_device,
	&fh_uart1_device,
	&fh_uart2_device,
	&fh_pinctrl_device,
	&fh_i2c0_device,
	&fh_i2c1_device,
	&fh_i2c2_device,
	&fh_rtc_device,
	&fh_sd0_device,
	&fh_sd1_device,
	&fh_sadc_device,
	&fh_gmac_device,
	&fh_gpio0_device,
	&fh_gpio1_device,
	&fh_aes_device,
	&fh_efuse_device,
#ifdef CONFIG_FH_DMAC
	&fh_dma_device,
#endif
#ifdef CONFIG_FH_AXI_DMAC
	&fh_axi_dma_device,
#endif
	&fh_spi0_device,
	&fh_spi1_device,
	&fh_spi2_device,
	&fh_i2s_device,
	&fh_pwm_device,
	&fh_wdt_device,
	&fh_usb_device,
#ifdef CONFIG_FH_PERF_MON
	&fh_perf_device,
#endif
#ifdef CONFIG_FH_TSENSOR
	&fh_tsensor_device,
#endif
};

static struct mtd_partition fh_sf_parts[] = {
	{
		/* head & Ramboot */
		.name		= "bootstrap",
		.offset		= 0,
		.size		= SZ_256K,
		.mask_flags	= MTD_WRITEABLE, /* force read-only */
	}, {
		/* Ramboot & U-Boot environment */
		.name		= "uboot-env",
		.offset		= MTDPART_OFS_APPEND,
		.size		= SZ_64K,
		.mask_flags	= MTD_WRITEABLE, /* force read-only */
	}, {
		/* U-Boot */
		.name		= "uboot",
		.offset		= MTDPART_OFS_APPEND,
		.size		= SZ_256K,
		.mask_flags	= MTD_WRITEABLE, /* force read-only */
	}, {
		.name		= "kernel",
		.offset		= MTDPART_OFS_APPEND,
		.size		= SZ_4M,
		.mask_flags	= 0,
	}, {
		.name		= "rootfs",
		.offset		= MTDPART_OFS_APPEND,
		.size		= SZ_8M,
		.mask_flags	= 0,
	}, {
		.name		= "app",
		.offset		= MTDPART_OFS_APPEND,
		.size		= MTDPART_SIZ_FULL,
		.mask_flags	= 0,
	}
	/* mtdparts=
	* spi_flash:256k(bootstrap),
	* 64k(u-boot-env),
	* 192k(u-boot),4M(kernel),
	* 8M(rootfs),
	* -(app) */
	/* two blocks with bad block table (and mirror) at the end */
};
#ifdef CONFIG_MTD_SPI_NAND
static struct mtd_partition fh_sf_nand_parts[] = {
	{
		/* head & Ramboot */
		.name		= "bootstrap",
		.offset		= 0,
		.size		= SZ_256K,
		.mask_flags	= MTD_WRITEABLE, /* force read-only */
	}, {
		.name		= "uboot-env",
		.offset		= MTDPART_OFS_APPEND,
		.size		= SZ_256K,
		.mask_flags	= MTD_WRITEABLE,
	}, {
		.name		= "uboot",
		.offset		= MTDPART_OFS_APPEND,
		.size		= SZ_512K,
		.mask_flags	= MTD_WRITEABLE,
	}, {
		.name		= "kernel",
		.offset		= MTDPART_OFS_APPEND,
		.size		= SZ_4M,
		.mask_flags	= 0,
	}, {
		.name		= "rootfs",
		.offset		= MTDPART_OFS_APPEND,
		.size		= SZ_8M,
		.mask_flags	= 0,
	}, {
		.name		= "app",
		.offset		= MTDPART_OFS_APPEND,
		.size		= MTDPART_SIZ_FULL,
		.mask_flags	= 0,
	}
	/* mtdparts=
	 * spi0.0:64k(bootstrap),
	 * 64k(u-boot-env),
	 * 192k(u-boot),
	 * 4M(kernel),
	 * 8M(rootfs),
	 * -(app)
	 * two blocks with bad block table (and mirror) at the end
	 */
};
#endif

static struct flash_platform_data fh_flash_platform_data  = {
	.name		= "spi_flash",
	.parts		= fh_sf_parts,
	.nr_parts	= ARRAY_SIZE(fh_sf_parts),
};
#ifdef CONFIG_MTD_SPI_NAND
static struct flash_platform_data fh_nandflash_platform_data  = {
	.name		= "spi_nandflash",
	.parts		= fh_sf_nand_parts,
	.nr_parts	= ARRAY_SIZE(fh_sf_nand_parts),
};
#endif

static struct spi_board_info fh_spi_devices[] = {
#ifdef CONFIG_MTD_SPI_NAND
	{
		.modalias		= "spi-nand",
		.bus_num		= 0,
		.chip_select	= 0,
		.max_speed_hz	= 50000000,
		.mode			= SPI_MODE_3,
		.platform_data	 = &fh_nandflash_platform_data,
	},
#endif
	{
		.modalias        = "m25p80",
		.bus_num         = 0,
		.chip_select     = 0,
		/* multi wire should adapt spi para 'ctl_wire_support'*/
		.mode            = SPI_MODE_3  | SPI_RX_DUAL,
		.max_speed_hz    = 50000000,
		.platform_data   = &fh_flash_platform_data,
	},

};

extern void early_print(const char *str, ...);

static void __init fh_console_pre_init(struct fh_platform_uart *plat, int num)
{
	int idx = 0;

	for (; idx < num; idx++) {
		struct uart_port *port;

		port            = &fh_serial_ports[idx];
		port->mapbase   = plat[idx].mapbase;
		port->fifosize  = plat[idx].fifo_size;
		port->uartclk   = plat[idx].uartclk;

		switch (idx) {
		case 0:
			port->membase = (unsigned char *)VA_UART0_REG_BASE;
			break;
		case 1:
			port->membase = (unsigned char *)VA_UART1_REG_BASE;
			break;
		case 2:
			port->membase = (unsigned char *)VA_UART2_REG_BASE;
			break;
		default:
			break;
		}
	}
}

static void __init fh8856v210_map_io(void)
{
	iotable_init(fh8856v210_io_desc, ARRAY_SIZE(fh8856v210_io_desc));
	fh_console_pre_init(fh_uart_platform_data,
			ARRAY_SIZE(fh_uart_platform_data));
}


static __init void fh8856v210_board_init(void)
{
	if (fh_is_8856v210())
		fh_rtc_device.dev.platform_data = &rtc_plat_data[1];
	platform_add_devices(fh8856v210_devices,
			ARRAY_SIZE(fh8856v210_devices));
	spi_register_board_info(fh_spi_devices, ARRAY_SIZE(fh_spi_devices));
}
void  __init fh_timer_init_no_of(unsigned int iovbase,
	unsigned int irqno);

static void __init fh8856v210_init_early(void)
{
	fh_pmu_init();
	fh_pinctrl_init(VA_PMU_REG_BASE + 0x80);
}

static void __init fh_time_init(void)
{
	unsigned int vtimerbase = (unsigned int)ioremap(TIMER_REG_BASE, SZ_4K);

	fh_clk_init();
	fh_timer_init_no_of(vtimerbase, TMR0_IRQ);

}

void __init fh_intc_init_no_of(unsigned int iovbase);
static void __init fh_intc_init(void)
{
	unsigned int vintcbase = (unsigned int)ioremap(INTC_REG_BASE, SZ_4K);

	fh_intc_init_no_of(vintcbase);

}
static void fh8856v210_restart
				(enum reboot_mode mode, const char *cmd)
{
	fh_pmu_restart();
}


MACHINE_START(FH8856V210, "FH8856V210")
	.atag_offset	= 0x100,
	.map_io		= fh8856v210_map_io,
	.init_irq	= fh_intc_init,
	.init_time	= fh_time_init,
	.init_machine	= fh8856v210_board_init,
	.init_early	= fh8856v210_init_early,
	.restart	= fh8856v210_restart,
MACHINE_END

