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
#include <mach/dma.h>


static u8  xm530_dma_peri[] = {
	DMACH_MAX,
	DMACH_MAX,
	DMACH_SPI0_TX,
	DMACH_SPI0_RX,
	DMACH_SPI1_TX,
	DMACH_SPI1_RX,
	DMACH_SPI2_TX,
	DMACH_SPI2_RX,
	DMACH_I2S,
	DMACH_UART0_TX,
	DMACH_UART0_RX,
	DMACH_UART1_TX,
	DMACH_UART1_RX,
	DMACH_UART2_TX,
	DMACH_UART2_RX,
	DMACH_I2S_TX,
	DMACH_I2S_RX,
	DMACH_MAX,
};

static struct dma_pl330_platdata xm530_dma_platdata = {
	.nr_valid_peri = ARRAY_SIZE(xm530_dma_peri),
	.peri_id = xm530_dma_peri,
};


static AMBA_AHB_DEVICE(xm530_dma, "dma-pl330", 0x00041330,
	DMAC_BASE, {DMAC_IRQ}, NULL);

static int __init xm530_dmac_init(void)
{
	dma_cap_set(DMA_SLAVE, xm530_dma_platdata.cap_mask);
	dma_cap_set(DMA_CYCLIC, xm530_dma_platdata.cap_mask);
	xm530_dma_device.dev.platform_data = &xm530_dma_platdata;
	amba_device_register(&xm530_dma_device, &iomem_resource);

	return 0;
}
arch_initcall(xm530_dmac_init);
