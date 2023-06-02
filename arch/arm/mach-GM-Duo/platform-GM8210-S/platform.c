#include <linux/platform_device.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/dma-mapping.h>
#include <linux/spi/spi.h>
#include <linux/spi/flash.h>
#include <asm/setup.h>
#include <asm/sizes.h>
#include <linux/module.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach-types.h>
#include <mach/platform/board.h>

/******************************************************************************
 * I2C devices
 *****************************************************************************/
#ifdef CONFIG_I2C0_IP
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
#endif

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

#ifdef CONFIG_I2C3_IP
/* i2c:3 */
static struct resource ftiic010_3_resources[] = {
	{
		.start  = I2C_FTI2C010_3_PA_BASE,
		.end    = I2C_FTI2C010_3_PA_LIMIT,
		.flags  = IORESOURCE_MEM,
	},
	{
		.start	= I2C_FTI2C010_3_IRQ,
		.end	= I2C_FTI2C010_3_IRQ,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device ftiic010_3_device = {
	.name		= "ftiic010",
	.id		    = 3,
	.num_resources	= ARRAY_SIZE(ftiic010_3_resources),
	.resource	= ftiic010_2_resources,
};
#endif

#ifdef CONFIG_I2C4_IP
/* i2c:4 */
static struct resource ftiic010_4_resources[] = {
	{
		.start  = I2C_FTI2C010_4_PA_BASE,
		.end    = I2C_FTI2C010_4_PA_LIMIT,
		.flags  = IORESOURCE_MEM,
	},
	{
		.start	= I2C_FTI2C010_4_IRQ,
		.end	= I2C_FTI2C010_4_IRQ,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device ftiic010_4_device = {
	.name		= "ftiic010",
	.id		    = 4,
	.num_resources	= ARRAY_SIZE(ftiic010_4_resources),
	.resource	= ftiic010_4_resources,
};
#endif

#ifdef CONFIG_I2C5_IP
/* i2c:4 */
static struct resource ftiic010_5_resources[] = {
	{
		.start  = I2C_FTI2C010_5_PA_BASE,
		.end    = I2C_FTI2C010_5_PA_LIMIT,
		.flags  = IORESOURCE_MEM,
	},
	{
		.start	= I2C_FTI2C010_5_IRQ,
		.end	= I2C_FTI2C010_5_IRQ,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device ftiic010_5_device = {
	.name		= "ftiic010",
	.id		    = 4,
	.num_resources	= ARRAY_SIZE(ftiic010_5_resources),
	.resource	= ftiic010_5_resources,
};
#endif

/******************************************************************************
 * GPIO devices
 *****************************************************************************/
#if defined(CONFIG_GPIO_FTGPIO010)
/* GPIO 0 */
static struct resource ftgpio010_0_resource[] = {
	{
		.start	= GPIO_FTGPIO010_0_PA_BASE,
		.end 	= GPIO_FTGPIO010_0_PA_LIMIT,
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
#endif /* CONFIG_GPIO_FTGPIO010 */
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
#ifdef CONFIG_I2C0_IP
	&ftiic010_0_device,
#endif
#ifdef CONFIG_I2C1_IP
	&ftiic010_1_device,
#endif
#ifdef CONFIG_I2C2_IP
	&ftiic010_2_device,
#endif
#ifdef CONFIG_I2C3_IP
	&ftiic010_3_device,
#endif
#ifdef CONFIG_I2C4_IP
	&ftiic010_4_device,
#endif
#ifdef CONFIG_I2C5_IP
    &ftiic010_5_device,
#endif
#if defined(CONFIG_GPIO_FTGPIO010)
    &ftgpio010_0_device,
    &ftgpio010_1_device,
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
