#include <linux/platform_device.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/spi/spi.h>
#include <linux/spi/flash.h>
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
 * MAC devices
 *****************************************************************************/
/*MAC 0*/
static struct resource ftmac110_0_resource[] = {
	{
         	.start  = MAC_FTMAC110_PA_BASE,
         	.end   	= MAC_FTMAC110_PA_LIMIT,
         	.flags  = IORESOURCE_MEM,
	},
	{
         	.start  = MAC_FTMAC110_IRQ,
         	.end   	= MAC_FTMAC110_IRQ,
         	.flags  = IORESOURCE_IRQ,
	},
};
static u64 ftmac100_0_dmamask = 0xFFFFFFUL;
static struct platform_device ftmac100_0_device = {
	.name		= "ftmac110",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(ftmac110_0_resource),
	.resource	= ftmac110_0_resource,
	.dev = {
		.dma_mask = &ftmac100_0_dmamask,
		.coherent_dma_mask = 0xFFFFFFFF,
	},	
};

/******************************************************************************
 * OTG devices
 *****************************************************************************/
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
	.id             = -1,
	.num_resources  = ARRAY_SIZE(fotg210_0_resources),
	.resource       = fotg210_0_resources,
	.dev = {
		.dma_mask = &fotg210_0_dmamask,
		.coherent_dma_mask = 0xFFFFFFFF,
	},
};

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

/* GPIO 2 */
static struct resource ftgpio010_2_resource[] = {
	{
		.start	= GPIO_FTGPIO010_2_PA_BASE,
		.end 	= GPIO_FTGPIO010_2_PA_LIMIT,
		.flags  = IORESOURCE_MEM
	},
	{
		.start	= GPIO_FTGPIO010_2_IRQ,
		.end	= GPIO_FTGPIO010_2_IRQ,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device ftgpio010_2_device = {
	.name	= "ftgpio010",
	.id		= 2,
	.num_resources	= ARRAY_SIZE(ftgpio010_2_resource),
	.resource	= ftgpio010_2_resource
};

/* ****************************************************************************
 * array contains all platform devices
 * ****************************************************************************/ 
static struct platform_device *gm_devices[] __initdata = 
{
	/* I2C */
	&ftiic010_0_device,
	/* MAC */	
	&ftmac100_0_device,	
	/* OTG */
	&fotg210_0_device,
	/* GPIO */
	&ftgpio010_0_device,
	&ftgpio010_1_device,
	&ftgpio010_2_device,
};

void __init platform_devices_init(void)
{
    /* add platform devices here 
     */
     
    /* will invoke platform_device_register() to register all platform devices 
     */
	platform_add_devices(gm_devices, ARRAY_SIZE(gm_devices));
}

