#include <linux/module.h>
#include <linux/platform_device.h>  
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/slab.h>
#include <linux/mmc/host.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/dw_mmc.h>
#include <linux/dma-mapping.h>
#include <mach/hardware.h>
#include <mach/irqs.h>
#include "dw_mmc.h"

#if 0
#define DW_MCI_CAPABILITIES (MMC_CAP_4_BIT_DATA | MMC_CAP_MMC_HIGHSPEED |\
		MMC_CAP_SD_HIGHSPEED | MMC_CAP_8_BIT_DATA |\
		MMC_CAP_SDIO_IRQ)
#endif

#define DW_MCI_CAPABILITIES (MMC_CAP_MMC_HIGHSPEED |  MMC_CAP_8_BIT_DATA)
#define DW_MCI_CAPABILITIES2 (MMC_CAP2_HS200_1_8V_SDR)


static struct resource dw_mci_resources[] = {
	[0] = { 
		.start = EMMC_BASE,
		.end   = EMMC_BASE + 0xffff,
		.flags = IORESOURCE_MEM,
	},  
	[1] = { 
		.start = EMMC_IRQ,
		.end   = EMMC_IRQ,
		.flags = IORESOURCE_IRQ,
	},  
};

static struct dw_mci_board dw_board_data = {
	.num_slots			= 1,
	.caps				= DW_MCI_CAPABILITIES,
    .caps2              = DW_MCI_CAPABILITIES2,
	.bus_hz				= 40 * 1000 * 1000,
	.detect_delay_ms		= 200,
	//.fifo_depth			= 512,
};

static struct platform_device dw_mci_device =
{
	.name = "dw_mmc",
	.id = 0,
	.num_resources = ARRAY_SIZE(dw_mci_resources),
	.resource = dw_mci_resources,
	.dev = { 
		.coherent_dma_mask = DMA_BIT_MASK(32),
		.platform_data = &dw_board_data,
	},  
};

static int __init dw_mci_driver_init(void)
{
	platform_device_register(&dw_mci_device);
	return 0;
}

static void __exit dw_mci_driver_exit(void)
{
	platform_device_unregister(&dw_mci_device);
}

MODULE_LICENSE("GPL");

module_init(dw_mci_driver_init);
module_exit(dw_mci_driver_exit);



