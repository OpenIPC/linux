#include <linux/dma-mapping.h>
#include <linux/etherdevice.h>
#include <linux/fh_efuse.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/mmc/host.h>
#include <linux/platform_device.h>
#include <linux/reboot.h>
#include <linux/serial_core.h>
#include <linux/sizes.h>
#include <linux/spi/flash.h>
#include <linux/spi/spi.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>

#include <mach/clock.h>
#ifdef CONFIG_FH_DMAC
#include <mach/fh_dmac_plat.h>
#endif
#include <mach/fh_efuse_plat.h>
#include <mach/fh_gmac_plat.h>
#include <mach/fh_gpio_plat.h>
#ifdef CONFIG_FH8626V100_AJL33PQ0866_MMC
#include <mach/fh_mci_plat.h>
#endif
#ifdef CONFIG_FH_DW_I2S
#include <mach/fh_i2s_plat.h>
#endif
#include <mach/fh_pwm_plat.h>
#ifdef CONFIG_FH8626V100_ONCHIP_RTC
#include <mach/fh_rtc_plat.h>
#endif
#include <mach/fh_sadc_plat.h>
#include <mach/fh_spi_plat.h>
#include <mach/fh_uart_plat.h>
#include <mach/fh_usb_plat.h>
#include <mach/fh_wdt_plat.h>
#include <mach/io.h>
#include <mach/pinctrl.h>
#include <mach/pmu.h>

#include "chip.h"
#include "platform.h"

struct uart_port fh_serial_ports[FH_UART_NUMBER];

static const u8 fh8626v100_ethaddr_sentinel[ETH_ALEN] __initconst = {
	0x10, 0x20, 0x30, 0x40, 0x50, 0x60
};

static const u8 openipc_ethaddr_sentinel[ETH_ALEN] __initconst = {
	0x00, 0x00, 0x23, 0x34, 0x45, 0x66
};

static u8 boot_ethaddr[ETH_ALEN] __initdata;

static bool __init fh8626v100_mac_is_usable(const u8 *addr)
{
	return is_valid_ether_addr(addr) &&
		!ether_addr_equal(addr, fh8626v100_ethaddr_sentinel) &&
		!ether_addr_equal(addr, openipc_ethaddr_sentinel);
}

static int __init fh8626v100_parse_ethaddr(char *str)
{
	if (str && mac_pton(str, boot_ethaddr) &&
	    fh8626v100_mac_is_usable(boot_ethaddr))
		return 0;

	eth_zero_addr(boot_ethaddr);
	pr_warn("fh8626v100: ignoring invalid or sentinel ethaddr bootarg\n");
	return 0;
}

early_param("ethaddr", fh8626v100_parse_ethaddr);

static struct map_desc fh8626v100_io_desc[] __initdata = {
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

static struct resource fh_uart0_resources[] = {
	{
		.start	= UART0_REG_BASE,
		.end	= UART0_REG_BASE + SZ_16K - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= UART0_IRQ,
		.end	= UART0_IRQ,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct resource fh_uart1_resources[] = {
	{
		.start	= UART1_REG_BASE,
		.end	= UART1_REG_BASE + SZ_16K - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= UART1_IRQ,
		.end	= UART1_IRQ,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct resource fh_uart2_resources[] = {
	{
		.start	= UART2_REG_BASE,
		.end	= UART2_REG_BASE + SZ_16K - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= UART2_IRQ,
		.end	= UART2_IRQ,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct fh_platform_uart fh_uart_platform_data[] = {
	{
		.mapbase	= UART0_REG_BASE,
		.fifo_size	= 16,
		.irq		= UART0_IRQ,
		.uartclk	= FH8626V100_UART_CLOCK,
		.use_dma	= 0,
		.dma_info	= NULL,
	},
	{
		.mapbase	= UART1_REG_BASE,
		.fifo_size	= 32,
		.irq		= UART1_IRQ,
		.uartclk	= FH8626V100_UART_CLOCK,
		.use_dma	= 0,
		.dma_info	= NULL,
	},
	{
		.mapbase	= UART2_REG_BASE,
		.fifo_size	= 32,
		.irq		= UART2_IRQ,
		.uartclk	= FH8626V100_UART_CLOCK,
		.use_dma	= 0,
		.dma_info	= NULL,
	},
};

static struct platform_device fh_uart0_device = {
	.name			= "ttyS",
	.id			= 0,
	.num_resources		= ARRAY_SIZE(fh_uart0_resources),
	.resource		= fh_uart0_resources,
	.dev.platform_data	= &fh_uart_platform_data[0],
};

static struct platform_device fh_uart1_device = {
	.name			= "ttyS",
	.id			= 1,
	.num_resources		= ARRAY_SIZE(fh_uart1_resources),
	.resource		= fh_uart1_resources,
	.dev.platform_data	= &fh_uart_platform_data[1],
};

static struct platform_device fh_uart2_device = {
	.name			= "ttyS",
	.id			= 2,
	.num_resources		= ARRAY_SIZE(fh_uart2_resources),
	.resource		= fh_uart2_resources,
	.dev.platform_data	= &fh_uart_platform_data[2],
};

static struct resource fh_gpio0_resources[] = {
	{
		.start	= GPIO0_REG_BASE,
		.end	= GPIO0_REG_BASE + SZ_16K - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= GPIO0_IRQ,
		.end	= GPIO0_IRQ,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct fh_gpio_chip fh_gpio0_chip = {
	.chip = {
		.owner	= THIS_MODULE,
		.label	= "FH_GPIO0",
		.base	= 0,
		.ngpio	= 32,
	},
};

static struct platform_device fh_gpio0_device = {
	.name			= GPIO_NAME,
	.id			= 0,
	.num_resources		= ARRAY_SIZE(fh_gpio0_resources),
	.resource		= fh_gpio0_resources,
	.dev.platform_data	= &fh_gpio0_chip,
};

static struct resource fh_gpio1_resources[] = {
	{
		.start	= GPIO1_REG_BASE,
		.end	= GPIO1_REG_BASE + SZ_16K - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= GPIO1_IRQ,
		.end	= GPIO1_IRQ,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct fh_gpio_chip fh_gpio1_chip = {
	.chip = {
		.owner	= THIS_MODULE,
		.label	= "FH_GPIO1",
		.base	= 32,
		.ngpio	= 32,
	},
};

static struct platform_device fh_gpio1_device = {
	.name			= GPIO_NAME,
	.id			= 1,
	.num_resources		= ARRAY_SIZE(fh_gpio1_resources),
	.resource		= fh_gpio1_resources,
	.dev.platform_data	= &fh_gpio1_chip,
};

static struct resource fh_gmac_resources[] = {
	{
		.start	= GMAC_REG_BASE,
		.end	= GMAC_REG_BASE + SZ_16K - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= GMAC_IRQ,
		.end	= GMAC_IRQ,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct fh_gmac_platform_data fh_gmac_data = {
	.phy_reset_pin	= FH8626V100_PHY_RESET_GPIO,
};

static void __init fh8626v100_select_mac_address(void)
{
	if (fh8626v100_mac_is_usable(boot_ethaddr)) {
		ether_addr_copy(fh_gmac_data.mac_addr, boot_ethaddr);
		pr_info("fh8626v100: using ethaddr bootarg MAC %pM\n",
			fh_gmac_data.mac_addr);
	} else {
		eth_zero_addr(fh_gmac_data.mac_addr);
		pr_info("fh8626v100: no usable bootloader MAC address\n");
	}
}

static struct platform_device fh_gmac_device = {
	.name			= "fh_gmac",
	.id			= 0,
	.num_resources		= ARRAY_SIZE(fh_gmac_resources),
	.resource		= fh_gmac_resources,
	.dev = {
		.coherent_dma_mask	= DMA_BIT_MASK(32),
		.platform_data		= &fh_gmac_data,
	},
};

static struct resource fh_i2c0_resources[] = {
	{
		.start	= I2C0_REG_BASE,
		.end	= I2C0_REG_BASE + SZ_16K - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= I2C0_IRQ,
		.end	= I2C0_IRQ,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct resource fh_i2c1_resources[] = {
	{
		.start	= I2C1_REG_BASE,
		.end	= I2C1_REG_BASE + SZ_16K - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= I2C1_IRQ,
		.end	= I2C1_IRQ,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct resource fh_i2c2_resources[] = {
	{
		.start	= I2C2_REG_BASE,
		.end	= I2C2_REG_BASE + SZ_16K - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= I2C2_IRQ,
		.end	= I2C2_IRQ,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device fh_i2c0_device = {
	.name			= "fh_i2c",
	.id			= 0,
	.num_resources		= ARRAY_SIZE(fh_i2c0_resources),
	.resource		= fh_i2c0_resources,
};

static struct platform_device fh_i2c1_device = {
	.name			= "fh_i2c",
	.id			= 1,
	.num_resources		= ARRAY_SIZE(fh_i2c1_resources),
	.resource		= fh_i2c1_resources,
};

static struct platform_device fh_i2c2_device = {
	.name			= "fh_i2c",
	.id			= 2,
	.num_resources		= ARRAY_SIZE(fh_i2c2_resources),
	.resource		= fh_i2c2_resources,
};

#ifdef CONFIG_FH8626V100_AJL33PQ0866_MMC
static struct resource fh_sdc0_resources[] = {
	{
		.start	= SDC0_REG_BASE,
		.end	= SDC0_REG_BASE + SZ_16K - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= SDC0_IRQ,
		.end	= SDC0_IRQ,
		.flags	= IORESOURCE_IRQ,
	},
};

/*
 * Exact AJL33PQ0866 stock SD0 data.  A zero caps field selects one-bit mode.
 * Card detect comes from the controller's dedicated SD0_CD input.  The
 * SD0_1BIT_NO_WP mux has no write-protect pad, and stock leaves get_ro unset.
 */
static struct fh_mci_board fh_mci_sd0_data = {
	.num_slots		= 1,
	.bus_hz			= 50000000,
	.detect_delay_ms	= 200,
	.drv_degree		= 2,
	.sam_degree		= 0,
};

static struct platform_device fh_sd0_device = {
	.name			= "fh_mci",
	.id			= 0,
	.num_resources		= ARRAY_SIZE(fh_sdc0_resources),
	.resource		= fh_sdc0_resources,
	.dev = {
		.coherent_dma_mask	= DMA_BIT_MASK(32),
		.platform_data		= &fh_mci_sd0_data,
	},
};
#endif

/*
 * Keep RTC registration in the generic FH8626V100 platform. Board profiles
 * without validated clock/backup hardware, including AJL33PQ0866, disable the
 * option in their board-only configuration rather than removing shared SoC
 * support.
 */
#ifdef CONFIG_FH8626V100_ONCHIP_RTC
static struct resource fh_rtc_resources[] = {
	{
		.start	= RTC_REG_BASE,
		.end	= RTC_REG_BASE + SZ_16K - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= RTC_IRQ,
		.end	= RTC_IRQ,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct fh_rtc_plat_data fh_rtc_data = {
	.lut_cof			= 58,
	.lut_offset			= 0xff,
	.tsensor_cp_default_out		= 0x993,
	.clk_name			= "rtc_pclk_gate",
};

static struct platform_device fh_rtc_device = {
	.name			= "fh_rtc",
	.id			= 0,
	.num_resources		= ARRAY_SIZE(fh_rtc_resources),
	.resource		= fh_rtc_resources,
	.dev.platform_data	= &fh_rtc_data,
};
#endif

static struct resource fh_pwm_resources[] = {
	{
		.start	= PWM_REG_BASE,
		.end	= PWM_REG_BASE + SZ_16K - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= PWM_IRQ,
		.end	= PWM_IRQ,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct fh_pwm_data fh_pwm_data = {
	.npwm	= 14,
};

static struct platform_device fh_pwm_device = {
	.name			= "fh_pwm",
	.id			= 0,
	.num_resources		= ARRAY_SIZE(fh_pwm_resources),
	.resource		= fh_pwm_resources,
	.dev.platform_data	= &fh_pwm_data,
};

static struct resource fh_sadc_resources[] = {
	{
		.start	= SADC_REG_BASE,
		.end	= SADC_REG_BASE + SZ_16K - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= SADC_IRQ,
		.end	= SADC_IRQ,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct fh_sadc_platform_data fh_sadc_data = {
	.ref_vol	= 3300,
	.active_bit	= 0xfff,
};

static struct platform_device fh_sadc_device = {
	.name			= "fh_sadc",
	.id			= 0,
	.num_resources		= ARRAY_SIZE(fh_sadc_resources),
	.resource		= fh_sadc_resources,
	.dev.platform_data	= &fh_sadc_data,
};

static struct resource fh_efuse_resources[] = {
	{
		.start	= EFUSE_REG_BASE,
		.end	= EFUSE_REG_BASE + SZ_16K - 1,
		.flags	= IORESOURCE_MEM,
	},
};

static struct fh_efuse_platform_data fh_efuse_data = {
	.efuse_support_flag	= CRYPTO_CPU_SET_KEY |
		CRYPTO_EX_MEM_SET_KEY |
		CRYPTO_EX_MEM_SWITCH_KEY |
		CRYPTO_EX_MEM_4_ENTRY_1_KEY |
		CRYPTO_EX_MEM_INDEP_POWER,
};

static struct platform_device fh_efuse_device = {
	.name			= "fh_efuse",
	.id			= 0,
	.num_resources		= ARRAY_SIZE(fh_efuse_resources),
	.resource		= fh_efuse_resources,
	.dev.platform_data	= &fh_efuse_data,
};

#ifdef CONFIG_FH_DMAC
/* Resources and six-channel priority policy recovered from the stock kernel. */
static struct resource fh_dma_resources[] = {
	{
		.start	= DMAC_REG_BASE,
		.end	= DMAC_REG_BASE + SZ_16K - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= DMAC0_IRQ,
		.end	= DMAC0_IRQ,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct fh_dma_platform_data fh_dma_data = {
	.nr_channels		= 6,
	.chan_priority		= CHAN_PRIORITY_ASCENDING,
	.clk_name		= "ahb_clk",
};

static struct platform_device fh_dma_device = {
	.name			= "fh_dmac",
	.id			= 0,
	.num_resources		= ARRAY_SIZE(fh_dma_resources),
	.resource		= fh_dma_resources,
	.dev.platform_data	= &fh_dma_data,
};
#endif

#ifdef CONFIG_FH_DW_I2S
/*
 * Internal-codec audio contract recovered from the exact FH8626V100 stock
 * kernel.  This path does not use the external DWI2S pads, so deliberately do
 * not add DWI2S to fh_pinctrl_selected_devices.
 */
static struct resource fh_i2s_resources[] = {
	{
		.start	= I2S_REG_BASE,
		.end	= I2S_REG_BASE + SZ_16K - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= ACW_REG_BASE,
		.end	= ACW_REG_BASE + SZ_16K - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= I2S0_IRQ,
		.end	= I2S0_IRQ,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct fh_i2s_platform_data fh_i2s_data = {
	.dma_capture_channel	= 2,
	.dma_playback_channel	= 3,
	.dma_master		= 1,
	.dma_rx_hs_num		= 10,
	.dma_tx_hs_num		= 11,
	.clk			= "i2s_clk",
	.pclk			= NULL,
	.acodec_pclk		= "acodec_pclk",
	.acodec_mclk		= "acodec_mclk",
};

static struct platform_device fh_i2s_device = {
	.name			= "fh_audio",
	.id			= 0,
	.num_resources		= ARRAY_SIZE(fh_i2s_resources),
	.resource		= fh_i2s_resources,
	.dev.platform_data	= &fh_i2s_data,
};
#endif

static struct resource fh_usb_resources[] = {
	{
		.start	= USBC_REG_BASE,
		.end	= USBC_REG_BASE + SZ_1M - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= USBC_IRQ,
		.end	= USBC_IRQ,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct fh_usb_platform_data fh_usb_data = {
	.dr_mode	= "host",
	.vbus_pwren	= 0xffffffff,
};

static struct platform_device fh_usb_device = {
	.name			= "fh_usb",
	.id			= 0,
	.num_resources		= ARRAY_SIZE(fh_usb_resources),
	.resource		= fh_usb_resources,
	.dev.platform_data	= &fh_usb_data,
};

static struct resource fh_wdt_resources[] = {
	{
		.start	= WDT_REG_BASE,
		.end	= WDT_REG_BASE + SZ_16K - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= WDT_IRQ,
		.end	= WDT_IRQ,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct fh_wdt_platform_data fh_wdt_data = {
	.mode	= MODE_DISCRETE,
};

static struct platform_device fh_wdt_device = {
	.name			= "fh_wdt",
	.id			= 0,
	.num_resources		= ARRAY_SIZE(fh_wdt_resources),
	.resource		= fh_wdt_resources,
	.dev.platform_data	= &fh_wdt_data,
};

static struct resource fh_spi0_resources[] = {
	{
		.start	= SPI0_REG_BASE,
		.end	= SPI0_REG_BASE + SZ_16K - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= SPI0_IRQ,
		.end	= SPI0_IRQ,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct fh_spi_platform_data fh_spi0_data = {
	.apb_clock_in		= FH8626V100_SPI0_APB_CLOCK,
	.slave_max_num		= 2,
	.clock_source		= { FH8626V100_SPI0_APB_CLOCK },
	.clock_source_num	= 1,
	.cs_data[0].GPIO_Pin	= FH8626V100_SPI0_CS0,
	.cs_data[0].name	= "spi0_cs0",
	.cs_data[1].GPIO_Pin	= FH8626V100_SPI0_CS1,
	.cs_data[1].name	= "spi0_cs1",
	.bus_no			= 0,
	.clk_name		= "spi0_clk",
	.ctl_wire_support	= ONE_WIRE_SUPPORT | DUAL_WIRE_SUPPORT |
				  MULTI_WIRE_SUPPORT,
	.dma_transfer_enable	= SPI_TRANSFER_USE_DMA,
	.rx_handshake_num	= 4,
};

static struct platform_device fh_spi0_device = {
	.name			= "fh_spi",
	.id			= 0,
	.num_resources		= ARRAY_SIZE(fh_spi0_resources),
	.resource		= fh_spi0_resources,
	.dev.platform_data	= &fh_spi0_data,
};

static struct mtd_partition fh8626v100_spi_parts[] = {
	/*
	 * Keep the factory 8 MiB geometry: U-Boot and existing recovery notes
	 * use these exact offsets. OpenIPC only changes the userspace-facing
	 * names of data/app so standard overlay and sysupgrade code find them.
	 *
	 * MTD_WRITEABLE in mask_flags removes write permission. Protect
	 * immutable bootstrap/U-Boot. The environment remains writable for
	 * fw_setenv; kernel/rootfs are writable for sysupgrade; rootfs_data is
	 * the persistent JFFS2 overlay. It combines the factory data+res ranges:
	 * OpenIPC does not consume the proprietary resource filesystem, while a
	 * full 1 MiB upper layer is materially safer than 512 KiB.
	 */
	{
		.name		= "bootstrap",
		.offset		= 0,
		.size		= SZ_64K,
		.mask_flags	= MTD_WRITEABLE,
	},
	{
		.name		= "uboot-env",
		.offset		= MTDPART_OFS_APPEND,
		.size		= SZ_64K,
	},
	{
		.name		= "uboot",
		.offset		= MTDPART_OFS_APPEND,
		.size		= SZ_256K - SZ_64K,
		.mask_flags	= MTD_WRITEABLE,
	},
	{
		.name		= "kernel",
		.offset		= MTDPART_OFS_APPEND,
		.size		= SZ_2M + SZ_1M,
	},
	{
		.name		= "rootfs_data",
		.offset		= MTDPART_OFS_APPEND,
		.size		= SZ_1M,
	},
	{
		.name		= "rootfs",
		.offset		= MTDPART_OFS_APPEND,
		.size		= MTDPART_SIZ_FULL,
	},
};

static struct flash_platform_data fh8626v100_flash_data = {
	.parts		= fh8626v100_spi_parts,
	.nr_parts	= ARRAY_SIZE(fh8626v100_spi_parts),
};

static struct spi_board_info fh8626v100_spi_devices[] = {
	{
		.modalias	= "m25p80",
		.bus_num	= 0,
		.chip_select	= 0,
		.max_speed_hz	= 50000000,
		.mode		= SPI_MODE_3,
		.platform_data	= &fh8626v100_flash_data,
	},
};

#ifdef CONFIG_FH_PINCTRL_MISC_DEV
/*
 * fh_pinctrl_init() installs the FH8626 pin database, while the misc driver
 * still needs a matching platform device before it can expose
 * /proc/driver/pinctrl.  Other non-DT Fullhan boards register the same device.
 * The ANJIA profile uses that userspace ABI to mux its board-specific GC1054
 * reset line before exporting GPIO5.
 */
static struct platform_device fh_pinctrl_device = {
	.name			= "fh_pinctrl",
	.id			= 0,
};
#endif

static struct platform_device *fh8626v100_devices[] __initdata = {
	&fh_uart0_device,
	&fh_uart1_device,
	&fh_uart2_device,
	&fh_gpio0_device,
	&fh_gpio1_device,
	&fh_i2c0_device,
	&fh_i2c1_device,
	&fh_i2c2_device,
#ifdef CONFIG_FH8626V100_AJL33PQ0866_MMC
	&fh_sd0_device,
#endif
#ifdef CONFIG_FH_PINCTRL_MISC_DEV
	&fh_pinctrl_device,
#endif
#ifdef CONFIG_FH8626V100_ONCHIP_RTC
	&fh_rtc_device,
#endif
	&fh_sadc_device,
	&fh_efuse_device,
#ifdef CONFIG_FH_DMAC
	&fh_dma_device,
#endif
#ifdef CONFIG_FH_DW_I2S
	&fh_i2s_device,
#endif
	&fh_usb_device,
	&fh_pwm_device,
	&fh_gmac_device,
	&fh_spi0_device,
	&fh_wdt_device,
};

static void __init fh_console_pre_init(struct fh_platform_uart *plat, int num)
{
	int idx;

	for (idx = 0; idx < num; idx++) {
		struct uart_port *port = &fh_serial_ports[idx];

		port->mapbase = plat[idx].mapbase;
		port->fifosize = plat[idx].fifo_size;
		port->uartclk = plat[idx].uartclk;

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
		}
	}
}

static void __init fh8626v100_map_io(void)
{
	iotable_init(fh8626v100_io_desc, ARRAY_SIZE(fh8626v100_io_desc));
	fh_console_pre_init(fh_uart_platform_data,
			    ARRAY_SIZE(fh_uart_platform_data));
}

static void __init fh8626v100_board_init(void)
{
	fh8626v100_select_mac_address();
	spi_register_board_info(fh8626v100_spi_devices,
				ARRAY_SIZE(fh8626v100_spi_devices));
	platform_add_devices(fh8626v100_devices,
			     ARRAY_SIZE(fh8626v100_devices));
}

static void __init fh8626v100_init_early(void)
{
	fh_pmu_init();
	fh_pinctrl_init(VA_PMU_REG_BASE + 0x80);
#ifdef CONFIG_FH8626V100_AJL33PQ0866_MMC
	/* AJL one-bit SD0 leaves pad66/SD0_DATA1 free for GPIO61 reset. */
	fh_pinctrl_sdev("SD0_1BIT_NO_WP", 0);
#endif
}

static void __init fh8626v100_time_init(void)
{
	unsigned int vtimerbase;

	vtimerbase = (unsigned int)ioremap(TIMER_REG_BASE, SZ_4K);
	fh_clk_init();
	fh_timer_init_no_of(vtimerbase, TMR0_IRQ);
}

static void __init fh8626v100_intc_init(void)
{
	unsigned int vintcbase;

	vintcbase = (unsigned int)ioremap(INTC_REG_BASE, SZ_4K);
	fh_intc_init_no_of(vintcbase);
}

static void fh8626v100_restart(enum reboot_mode mode, const char *cmd)
{
	fh_pmu_restart();
}

MACHINE_START(FH8626V100, "FH8626V100")
	.atag_offset	= 0x100,
	.map_io		= fh8626v100_map_io,
	.init_irq	= fh8626v100_intc_init,
	.init_time	= fh8626v100_time_init,
	.init_machine	= fh8626v100_board_init,
	.init_early	= fh8626v100_init_early,
	.restart	= fh8626v100_restart,
MACHINE_END
