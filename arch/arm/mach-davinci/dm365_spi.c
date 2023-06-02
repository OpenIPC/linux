/*
 * TI DaVinci DM3xx SPI setup
 *
 * Copyright (C) 2010 Basler Vision Technologies AG
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

#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/ioport.h>
#include <linux/spi/spi.h>
#include <mach/mux.h>
#include <mach/irqs.h>

#include "dm365_spi.h"

static u64 davinci_spi_dma_mask = DMA_BIT_MASK(32);

enum davinci_spi_resource_index {
	spirsrc_iomem,
	spirsrc_irq,
	spirsrc_rxdma,
	spirsrc_txdma,
	spirsrc_evqdma
};


static struct resource davinci_spi_resources[spirsrc_evqdma + 1][5] = {
	{
		[spirsrc_iomem] = {
			.start	= 0x01c66000,
			.end	= 0x01c667ff,
			.flags	= IORESOURCE_MEM,
		},
		[spirsrc_irq] = {
			.flags	= IORESOURCE_IRQ,
		},
		[spirsrc_rxdma] = {
			.flags	= IORESOURCE_DMA | IORESOURCE_DMA_RX_CHAN,
		},
		[spirsrc_txdma] = {
			.flags	= IORESOURCE_DMA | IORESOURCE_DMA_TX_CHAN,
		},
		[spirsrc_evqdma] = {
			.flags	= IORESOURCE_DMA | IORESOURCE_DMA_EVENT_Q,
		}
	},
	{
		[spirsrc_iomem] = {
			.start	= 0x01c66800,
			.end	= 0x01c66fff,
			.flags	= IORESOURCE_MEM,
		},
		[spirsrc_irq] = {
			.flags	= IORESOURCE_IRQ,
		},
		[spirsrc_rxdma] = {
			.flags	= IORESOURCE_DMA
		},
		[spirsrc_txdma] = {
			.flags	= IORESOURCE_DMA
		},
		[spirsrc_evqdma] = {
			.flags	= IORESOURCE_DMA
		}
	},
	{
		[spirsrc_iomem] = {
			.start	= 0x01c67800,
			.end	= 0x01c67fff,
			.flags	= IORESOURCE_MEM,
		},
		[spirsrc_irq] = {
			.flags	= IORESOURCE_IRQ,
		},
		[spirsrc_rxdma] = {
			.flags	= IORESOURCE_DMA
		},
		[spirsrc_txdma] = {
			.flags	= IORESOURCE_DMA
		},
		[spirsrc_evqdma] = {
			.flags	= IORESOURCE_DMA
		}
	},
	{
		[spirsrc_iomem] = {
			.start	= 0x01c68000,
			.end	= 0x01c687ff,
			.flags	= IORESOURCE_MEM,
		},
		[spirsrc_irq] = {
			.flags	= IORESOURCE_IRQ,
			.start	= IRQ_DM365_SPIINT3_0
		},
		[spirsrc_rxdma] = {
			.flags	= IORESOURCE_DMA
		},
		[spirsrc_txdma] = {
			.flags	= IORESOURCE_DMA
		},
		[spirsrc_evqdma] = {
			.flags	= IORESOURCE_DMA
		}
	},
	{
		[spirsrc_iomem] = {
			.start	= 0x01c23000,
			.end	= 0x01c237ff,
			.flags	= IORESOURCE_MEM,
		},
		[spirsrc_irq] = {
			.flags	= IORESOURCE_IRQ,
		},
		[spirsrc_rxdma] = {
			.flags	= IORESOURCE_DMA
		},
		[spirsrc_txdma] = {
			.flags	= IORESOURCE_DMA
		},
		[spirsrc_evqdma] = {
			.flags	= IORESOURCE_DMA
		}
	}
};

static struct platform_device davinci_spi_device[] = {
	{
		.name = "spi_davinci",
		.id = 0,
		.dev = {
			.dma_mask = &davinci_spi_dma_mask,
			.coherent_dma_mask = DMA_BIT_MASK(32),
		},
		.num_resources = ARRAY_SIZE(davinci_spi_resources[0]),
		.resource = davinci_spi_resources[0]
	},
	{
		.name = "spi_davinci",
		.id = 1,
		.dev = {
			.dma_mask = &davinci_spi_dma_mask,
			.coherent_dma_mask = DMA_BIT_MASK(32),
		},
		.num_resources = ARRAY_SIZE(davinci_spi_resources[1]),
		.resource = davinci_spi_resources[1]
	},
	{
		.name = "spi_davinci",
		.id = 2,
		.dev = {
			.dma_mask = &davinci_spi_dma_mask,
			.coherent_dma_mask = DMA_BIT_MASK(32),
		},
		.num_resources = ARRAY_SIZE(davinci_spi_resources[2]),
		.resource = davinci_spi_resources[2]
	},
	{
		.name = "spi_davinci",
		.id = 3,
		.dev = {
			.dma_mask = &davinci_spi_dma_mask,
			.coherent_dma_mask = DMA_BIT_MASK(32),
		},
		.num_resources = ARRAY_SIZE(davinci_spi_resources[3]),
		.resource = davinci_spi_resources[3]
	},
	{
		.name = "spi_davinci",
		.id = 4,
		.dev = {
			.dma_mask = &davinci_spi_dma_mask,
			.coherent_dma_mask = DMA_BIT_MASK(32),
		},
		.num_resources = ARRAY_SIZE(davinci_spi_resources[4]),
		.resource = davinci_spi_resources[4]
	}
};

struct davinci_spi_pins {
	int	sclk;
	int	sdi;
	int	sdo;
	int	sdena0;
	int	sdena1;
};

static const struct davinci_spi_pins davinci_spi_pinmap[] __initconst = {
	{
		.sclk	= DM365_SPI0_SCLK,
		.sdi	= DM365_SPI0_SDI,
		.sdo	= DM365_SPI0_SDO,
		.sdena0	= DM365_SPI0_SDENA0,
		.sdena1	= DM365_SPI0_SDENA1
	},
	{
		.sclk	= DM365_SPI1_SCLK,
		.sdi	= DM365_SPI1_SDI,
		.sdo	= DM365_SPI1_SDO,
		.sdena0	= DM365_SPI1_SDENA0,
		.sdena1	= DM365_SPI1_SDENA1
	},
	{
		.sclk	= DM365_SPI2_SCLK,
		.sdi	= DM365_SPI2_SDI,
		.sdo	= DM365_SPI2_SDO,
		.sdena0	= DM365_SPI2_SDENA0,
		.sdena1	= DM365_SPI2_SDENA1
	},
	{
		.sclk	= DM365_SPI3_SCLK,
		.sdi	= DM365_SPI3_SDI,
		.sdo	= DM365_SPI3_SDO,
		.sdena0	= DM365_SPI3_SDENA0,
		.sdena1	= DM365_SPI3_SDENA1
	},
	{
		.sclk	= DM365_SPI4_SCLK,
		.sdi	= DM365_SPI4_SDI,
		.sdo	= DM365_SPI4_SDO,
		.sdena0	= DM365_SPI4_SDENA0,
		.sdena1	= DM365_SPI4_SDENA1
	}
};

void __init davinci_init_spi(struct davinci_spi_unit_desc *unit,
			unsigned int ninfo,
			const struct spi_board_info *info)
{
	int err;
	const unsigned int hwunit = unit->spi_hwunit;
	const struct davinci_spi_pins * const pins = &davinci_spi_pinmap[hwunit];
	struct platform_device * const pdev = &davinci_spi_device[hwunit];
	struct davinci_spi_platform_data * const pdata = &unit->pdata;

	davinci_cfg_reg(pins->sclk);
	davinci_cfg_reg(pins->sdi);
	davinci_cfg_reg(pins->sdo);

	/* not all slaves will be wired up */
	if (unit->chipsel & BIT(0))
		davinci_cfg_reg(pins->sdena0);
	if (unit->chipsel & BIT(1))
		davinci_cfg_reg(pins->sdena1);

	pdev->dev.platform_data = pdata;

	pdev->resource[spirsrc_irq].start =
	pdev->resource[spirsrc_irq].end = unit->irq;
	pdev->resource[spirsrc_rxdma].start =
	pdev->resource[spirsrc_rxdma].end = unit->dma_rx_chan;
	pdev->resource[spirsrc_txdma].start =
	pdev->resource[spirsrc_txdma].end = unit->dma_tx_chan;
	pdev->resource[spirsrc_evqdma].start =
	pdev->resource[spirsrc_evqdma].end = unit->dma_evtq;

	pr_debug("Creating SPI%u: irq = %u, dma_rx = %u, dma_tx = %u, "
		"dma_evq = %u",
		hwunit, unit->irq, unit->dma_rx_chan, unit->dma_tx_chan,
		unit->dma_evtq);

	err = platform_device_register(pdev);
	if (unlikely(err))
		pr_err("Failed to create platform device for SPI%u - error %d",
			hwunit, err);

	spi_register_board_info(info, ninfo);
}

