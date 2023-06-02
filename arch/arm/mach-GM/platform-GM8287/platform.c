#include <linux/platform_device.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/spi/spi.h>
#include <linux/spi/flash.h>
#include <linux/dma-mapping.h>
#include <asm/setup.h>
#include <asm/sizes.h>
#include <linux/module.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach-types.h>

/******************************************************************************
 * I2C devices
 *****************************************************************************/
/* i2c:0 */
static struct resource ftiic010_0_resources[] = {
	{
		.start  = I2C_FTI2C010_0_PA_BASE,
		.end    = I2C_FTI2C010_0_PA_LIMIT,
		.flags  = IORESOURCE_MEM,
	},
	{
		.start	= I2C_FTI2C010_0_IRQ,
		.end	= I2C_FTI2C010_0_IRQ,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device ftiic010_0_device = {
	.name		= "ftiic010",
	.id		    = 0,
	.num_resources	= ARRAY_SIZE(ftiic010_0_resources),
	.resource	= ftiic010_0_resources,
};

#ifdef CONFIG_I2C1_IP
/* i2c:1 */
static struct resource ftiic010_1_resources[] = {
	{
		.start  = I2C_FTI2C010_1_PA_BASE,
		.end    = I2C_FTI2C010_1_PA_LIMIT,
		.flags  = IORESOURCE_MEM,
	},
	{
		.start	= I2C_FTI2C010_1_IRQ,
		.end	= I2C_FTI2C010_1_IRQ,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device ftiic010_1_device = {
	.name		= "ftiic010",
	.id		    = 1,
	.num_resources	= ARRAY_SIZE(ftiic010_1_resources),
	.resource	= ftiic010_1_resources,
};
#endif

#ifdef CONFIG_I2C2_IP
/* i2c:2 */
static struct resource ftiic010_2_resources[] = {
	{
		.start  = I2C_FTI2C010_2_PA_BASE,
		.end    = I2C_FTI2C010_2_PA_LIMIT,
		.flags  = IORESOURCE_MEM,
	},
	{
		.start	= I2C_FTI2C010_2_IRQ,
		.end	= I2C_FTI2C010_2_IRQ,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device ftiic010_2_device = {
	.name		= "ftiic010",
	.id		    = 2,
	.num_resources	= ARRAY_SIZE(ftiic010_2_resources),
	.resource	= ftiic010_2_resources,
};
#endif

/******************************************************************************
 * OTG devices
 *****************************************************************************/
#ifdef CONFIG_USB_SUPPORT
/* OTG:0 */
static struct resource fotg210_0_resources[] = {
	{
		.start  = USB_FOTG2XX_0_PA_BASE,
		.end    = USB_FOTG2XX_0_PA_LIMIT,
		.flags  = IORESOURCE_MEM,
	},
	{
		.start  = USB_FOTG2XX_0_IRQ,
		.end    = USB_FOTG2XX_0_IRQ,
		.flags  = IORESOURCE_IRQ,
	},
};
static u64 fotg210_0_dmamask = 0xFFFFFFUL;
static struct platform_device fotg210_0_device = {
	.name           = "fotg210",
	.id             = 0,
	.num_resources  = ARRAY_SIZE(fotg210_0_resources),
	.resource       = fotg210_0_resources,
	.dev = {
		.dma_mask = &fotg210_0_dmamask,
		.coherent_dma_mask = 0xFFFFFFFF,
	},
};
/* OTG:1 */
struct resource fotg210_1_resources[] = {
    {
        .start  = USB_FOTG2XX_1_PA_BASE,
        .end    = USB_FOTG2XX_1_PA_LIMIT,
        .flags  = IORESOURCE_MEM,
    },
    {
        .start  = USB_FOTG2XX_1_IRQ,
        .end    = USB_FOTG2XX_1_IRQ,
        .flags  = IORESOURCE_IRQ,
    },
};
static u64 fotg210_1_dmamask = 0xFFFFFFUL;
static struct platform_device fotg210_1_device = {
    .name           = "fotg210",
    .id             = 1,
    .num_resources  = ARRAY_SIZE(fotg210_1_resources),
    .resource       = fotg210_1_resources,
    .dev = {
        .dma_mask = &fotg210_1_dmamask,
        .coherent_dma_mask = 0xFFFFFFFF,
    },
};
#endif /* CONFIG_USB_SUPPORT */

/******************************************************************************
 * SDC devices
 *****************************************************************************/
#if defined(CONFIG_MMC_FTSDC021)
/* FTSDC021 */
static struct resource ftsdc021_resource[] = {
	[0] = {
	       .start = SDC_FTSDC021_PA_BASE,
	       .end = SDC_FTSDC021_PA_LIMIT,
	       .flags = IORESOURCE_MEM,
	       },
	[1] = {
	       .start = SDC_FTSDC021_0_IRQ,
	       .end = SDC_FTSDC021_0_IRQ,
	       .flags = IORESOURCE_IRQ,
	       }
};
static u64 ftsdc021_dmamask = 0xFFFFFFUL;
static struct platform_device ftsdc021_device = {
	.name = "ftsdc021",
	.id = 0,
	.num_resources = ARRAY_SIZE(ftsdc021_resource),
	.resource = ftsdc021_resource,
	.dev = {
		.dma_mask = &ftsdc021_dmamask,
		.coherent_dma_mask = 0xFFFFFFFF,
	},
};
#endif
/******************************************************************************
 * GPIO devices
 *****************************************************************************/
/* GPIO 0 */
static struct resource ftgpio010_0_resource[] = {
	{
		.start	= GPIO_FTGPIO010_PA_BASE,
		.end 	= GPIO_FTGPIO010_PA_LIMIT,
		.flags  = IORESOURCE_MEM
	},
	{
		.start	= GPIO_FTGPIO010_0_IRQ,
		.end	= GPIO_FTGPIO010_0_IRQ,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device ftgpio010_0_device = {
	.name	= "ftgpio010",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(ftgpio010_0_resource),
	.resource	= ftgpio010_0_resource
};

/* GPIO 1 */
static struct resource ftgpio010_1_resource[] = {
	{
		.start	= GPIO_FTGPIO010_1_PA_BASE,
		.end 	= GPIO_FTGPIO010_1_PA_LIMIT,
		.flags  = IORESOURCE_MEM
	},
	{
		.start	= GPIO_FTGPIO010_1_IRQ,
		.end	= GPIO_FTGPIO010_1_IRQ,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device ftgpio010_1_device = {
	.name	= "ftgpio010",
	.id		= 1,
	.num_resources	= ARRAY_SIZE(ftgpio010_1_resource),
	.resource	= ftgpio010_1_resource
};

/******************************************************************************
 * SATA devices
 *****************************************************************************/
#ifdef CONFIG_SATA_AHCI_PLATFORM
/* SATA0 */
static struct resource ftsata100_0_resource[] = {
    {
        .start = SATA_FTSATA100_0_PA_BASE,
        .end   = SATA_FTSATA100_0_PA_LIMIT,
        .flags = IORESOURCE_MEM,
    },
    {
        .start = SATA_FTSATA100_0_IRQ,
        .end   = SATA_FTSATA100_0_IRQ,
        .flags = IORESOURCE_IRQ,
    }
};

static u64 ftsata100_0_dmamask = ~(u32)0;
static struct platform_device ftsata100_0_device = {
	.name           = "ftsata100",
	.id             = 0,
	.num_resources  = ARRAY_SIZE(ftsata100_0_resource),
	.resource       = ftsata100_0_resource,
	.dev            = {
				.dma_mask = &ftsata100_0_dmamask,
				.coherent_dma_mask = 0xFFFFFFFF,
	},
};

/* SATA1 */
static struct resource ftsata100_1_resource[] = {
    {
        .start = SATA_FTSATA100_1_PA_BASE,
        .end   = SATA_FTSATA100_1_PA_LIMIT,
        .flags = IORESOURCE_MEM,
    },
    {
        .start = SATA_FTSATA100_1_IRQ,
        .end   = SATA_FTSATA100_1_IRQ,
        .flags = IORESOURCE_IRQ,
    }
};

static u64 ftsata100_1_dmamask = ~(u32)0;
static struct platform_device ftsata100_1_device = {
	.name           = "ftsata100",
	.id             = 1,
	.num_resources  = ARRAY_SIZE(ftsata100_1_resource),
	.resource       = ftsata100_1_resource,
	.dev            = {
				.dma_mask = &ftsata100_1_dmamask,
				.coherent_dma_mask = 0xFFFFFFFF,
	},
};
#endif // #ifdef CONFIG_SATA_AHCI_PLATFORM

/******************************************************************************
 * AHB DMA controllers
 *****************************************************************************/
#ifdef CONFIG_FTDMAC020
static struct resource ftdmac020_0_resources[] = {
	{
		.start	= DMAC_FTDMAC020_0_PA_BASE,
		.end	= DMAC_FTDMAC020_0_PA_LIMIT,
		.flags	= IORESOURCE_MEM,
	}, {
		.start	= DMAC_FTDMAC020_0_IRQ,
		.end    = DMAC_FTDMAC020_0_IRQ,
		.flags	= IORESOURCE_IRQ,
	}
};

static struct platform_device ftdmac020_0_device = {
	.name		= "ftdmac020",
	.id		    = 0,
	.dev		= {
		.coherent_dma_mask	= ((1ULL<<(32))-1),
	},
	.num_resources	= ARRAY_SIZE(ftdmac020_0_resources),
	.resource	= ftdmac020_0_resources,
};
#endif /* CONFIG_FTDMAC020 */

/******************************************************************************
 * SPI020 controllers
 *****************************************************************************/
#ifdef CONFIG_SPI_FTSPI020
static struct resource ftspi020_resource[] = {
    [0] = {
           .start = SPI_FTSPI020_PA_BASE,     /* Register Base address */
           .end = SPI_FTSPI020_PA_LIMIT,
           .flags = IORESOURCE_MEM,
           },
    [1] = {
           .start = SPI_FTSPI020_IRQ,
           .end = SPI_FTSPI020_IRQ,
           .flags = IORESOURCE_IRQ,
           }
};

static u64 ftspi020_dmamask = DMA_BIT_MASK(32);

static struct platform_device ftspi020_device = {
    .name = "ftspi020",
    .id = 0,//must match with bus_num
    .num_resources = ARRAY_SIZE(ftspi020_resource),
    .resource = ftspi020_resource,
    .dev = {
            .dma_mask = &ftspi020_dmamask,
            .coherent_dma_mask = DMA_BIT_MASK(32),
            },
};
#endif /* CONFIG_SPI_FTSPI020 */

/* ****************************************************************************
 * array contains all platform devices
 * ****************************************************************************/
static struct platform_device *gm_devices[] __initdata =
{
	/* I2C */
	&ftiic010_0_device,
#ifdef CONFIG_I2C1_IP
	&ftiic010_1_device,
#endif
#ifdef CONFIG_I2C2_IP
	&ftiic010_2_device,
#endif
	/* OTG */
#ifdef CONFIG_USB_SUPPORT
	&fotg210_0_device,
	&fotg210_1_device,
#endif
	/* SDC */
#if defined(CONFIG_MMC_FTSDC021)
    &ftsdc021_device,
#endif
	/* GPIO */
	&ftgpio010_0_device,
	&ftgpio010_1_device,
#ifdef CONFIG_SATA_AHCI_PLATFORM
    &ftsata100_0_device,
    &ftsata100_1_device,
#endif
#ifdef CONFIG_FTDMAC020
    &ftdmac020_0_device,
#endif
#ifdef CONFIG_SPI_FTSPI020
    &ftspi020_device,
#endif
};

void __init platform_devices_init(void)
{
    /* add platform devices here
     */

    /* will invoke platform_device_register() to register all platform devices
     */
	platform_add_devices(gm_devices, ARRAY_SIZE(gm_devices));
}
