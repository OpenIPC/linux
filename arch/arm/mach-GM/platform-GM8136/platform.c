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
#endif /* CONFIG_USB_SUPPORT */

/******************************************************************************
 * SDC devices
 *****************************************************************************/
/* FTSDC021 */
#ifdef CONFIG_MMC_FTSDC021
static u64 ftsdc021_dmamask = 0xFFFFFFUL;
#endif
#if defined(CONFIG_SDC0_IP)
static struct resource ftsdc021_0_resource[] = {
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
static struct platform_device ftsdc021_0_device = {
	.name = "ftsdc021",
	.id = 0,
	.num_resources = ARRAY_SIZE(ftsdc021_0_resource),
	.resource = ftsdc021_0_resource,
	.dev = {
		.dma_mask = &ftsdc021_dmamask,
		.coherent_dma_mask = 0xFFFFFFFF,
	},
};
#endif
#if defined(CONFIG_SDC1_IP)
static struct resource ftsdc021_1_resource[] = {
        [0] = {
               .start = SDC_FTSDC021_1_PA_BASE,
               .end = SDC_FTSDC021_1_PA_LIMIT,
               .flags = IORESOURCE_MEM,
               },
        [1] = {
               .start = SDC_FTSDC021_1_IRQ,
               .end = SDC_FTSDC021_1_IRQ,
               .flags = IORESOURCE_IRQ,
               }
};
static struct platform_device ftsdc021_1_device = {
        .name = "ftsdc021",
        .id = 1,
        .num_resources = ARRAY_SIZE(ftsdc021_1_resource),
        .resource = ftsdc021_1_resource,
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

#ifdef CONFIG_SPI_FTSSP010
static struct resource ftssp010_1_resources[] = {
	{
		.start	= SSP_FTSSP010_1_PA_BASE,
		.end	= SSP_FTSSP010_1_PA_LIMIT,
		.flags	= IORESOURCE_MEM,
	}, {
		.start	= SSP_FTSSP010_1_IRQ,
		.end    = SSP_FTSSP010_1_IRQ,
		.flags	= IORESOURCE_IRQ,
	}
};

static struct platform_device ftssp010_1_device = {
	.name		= "ssp_spi",
	.id		    = 1,
	.num_resources	= ARRAY_SIZE(ftssp010_1_resources),
	.resource	= ftssp010_1_resources,	
	.dev		= {
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	},
};

static struct spi_board_info spi_devs_info[] __initdata = {
    {
            .modalias       = "spidev",//"imx138_spi",
            .max_speed_hz   = 6000000,
            .bus_num        = 1,    //use controller 1
            .chip_select    = 0,    //chip select 0
            .mode = SPI_MODE_3,
    },
#if 0
    {
            .modalias       = "device_1_spi",
            .max_speed_hz   = 2000000,  //2MHz
            .bus_num        = 1,    //use controller 1
            .chip_select    = 1,    //chip select 1
            .mode = SPI_MODE_3,
    },
    {
            .modalias       = "device_2_spi",
            .max_speed_hz   = 3000000,  //3MHz
            .bus_num        = 1,    //use controller 1
            .chip_select    = 2,    //chip select 2
            .mode = SPI_MODE_3,
    },
#endif
};
#endif /* CONFIG_SPI_FTSSP010 */
/* ****************************************************************************
 * array contains all platform devices
 * ****************************************************************************/
static struct platform_device *gm_devices[] __initdata =
{
	/* I2C */
	&ftiic010_0_device,
	/* OTG */
#ifdef CONFIG_USB_SUPPORT
	&fotg210_0_device,
#endif
#if defined(CONFIG_SDC0_IP)
    &ftsdc021_0_device,
#endif
#if defined(CONFIG_SDC1_IP)
    &ftsdc021_1_device,
#endif
	/* GPIO */
	&ftgpio010_0_device,
	&ftgpio010_1_device,
#ifdef CONFIG_FTDMAC020
    &ftdmac020_0_device,
#endif
#ifdef CONFIG_SPI_FTSPI020
    &ftspi020_device,
#endif
#ifdef CONFIG_SPI_FTSSP010
    &ftssp010_1_device,
#endif
};

void __init platform_devices_init(void)
{
    /* add platform devices here
     */

    /* will invoke platform_device_register() to register all platform devices
     */
	platform_add_devices(gm_devices, ARRAY_SIZE(gm_devices));
#ifdef CONFIG_SPI_FTSSP010
	spi_register_board_info(spi_devs_info, ARRAY_SIZE(spi_devs_info));
#endif
}
