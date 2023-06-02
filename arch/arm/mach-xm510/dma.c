#include <linux/dma-mapping.h>
#include <linux/amba/bus.h>
#include <linux/amba/pl330.h>

#include <asm/mach/arch.h>
#include <asm/mach/flash.h>
#include <asm/mach/irq.h>
#include <asm/mach/time.h>
#include <asm/mach/map.h>

#include <mach/time.h>
#include <mach/hardware.h>
#include <mach/irqs.h>
#include <linux/clkdev.h>
#include "clock.h"
#include <mach/dma.h>


static struct dma_pl330_peri  xm510_dma_peri[] = {
	[0] = {
		.peri_id = 0,
		.rqtype = MEMTOMEM,
	},
	[1] = {
		.peri_id = 1,
		.rqtype = MEMTOMEM,
	},
	[2] = {
		.peri_id = DMACH_SPI0_TX,
		.rqtype = MEMTODEV,
	},
	[3] = {
		.peri_id = DMACH_SPI0_RX,
		.rqtype = DEVTOMEM,
	},
	[4] = {
		.peri_id = DMACH_SPI1_TX,
		.rqtype = MEMTODEV,
	},
	[4] = {
		.peri_id = DMACH_SPI1_RX,
		.rqtype = DEVTOMEM,
	},
	[6] = {
		.peri_id = DMACH_SPI2_TX,
		.rqtype = MEMTODEV,
	},
	[7] = {
		.peri_id = DMACH_SPI2_RX,
		.rqtype = DEVTOMEM,
	},
	[8] = {
		.peri_id = DMACH_I2S,
		.rqtype = DEVTOMEM,
	},
	[9] = {
		.peri_id = DMACH_UART0_TX,
		.rqtype = MEMTODEV,
	},
	[10] = {
		.peri_id = DMACH_UART0_RX,
		.rqtype = DEVTOMEM,
	},
	[11] = {
		.peri_id = DMACH_UART1_TX,
		.rqtype = MEMTODEV,
	},
	[12] = {
		.peri_id = DMACH_UART1_RX,
		.rqtype = DEVTOMEM,
	},
	[13] = {
		.peri_id = DMACH_UART2_TX,
		.rqtype = MEMTODEV,
	},
	[14] = {
		.peri_id = DMACH_UART2_RX,
		.rqtype = DEVTOMEM,
	},
	[15] = {
		.peri_id = DMACH_I2S_TX,
		.rqtype = MEMTODEV,
	},
	[16] = {
		.peri_id = DMACH_I2S_RX,
		.rqtype = DEVTOMEM,
	},
	[17] = {
		.peri_id = DMACH_MAX,
		.rqtype = MEMTOMEM,
	},
};

static struct dma_pl330_platdata xm510_dma_platdata = {
	.nr_valid_peri = ARRAY_SIZE(xm510_dma_peri),
	.peri = &xm510_dma_peri[0],
};


#define XM_AMBADEV_NAME(name) xm_ambadevice_##name

#define XM_AMBA_DEVICE(name, busid, base, platdata)			\
	static struct amba_device XM_AMBADEV_NAME(name) =		\
	{\
		.dev            = {                                     \
			.coherent_dma_mask = ~0,                        \
			.init_name = busid,                             \
			.platform_data = platdata,                      \
		},                                                      \
		.res            = {                                     \
			.start  = base##_BASE,				\
			.end    = base##_BASE + 0x1000 - 1,		\
			.flags  = IORESOURCE_IO,                        \
		},                                                      \
		.dma_mask       = ~0,                                   \
		.irq            = { base##_IRQ, NO_IRQ }		\
	}

XM_AMBA_DEVICE(dmac, "dmac",  DMAC, &xm510_dma_platdata);

static struct clk dmac_clk = {
	.rate   = 50000000,
};

static struct clk_lookup lookups[] = {
	{       /* DMAC */
		.dev_id         = "dmac",
		.clk            = &dmac_clk,
	},
};
static int __init xm510_dmac_init(void)
{
	clkdev_add_table(lookups, ARRAY_SIZE(lookups));
	amba_device_register(&XM_AMBADEV_NAME(dmac), &iomem_resource);

	return 0;
}
arch_initcall(xm510_dmac_init);
