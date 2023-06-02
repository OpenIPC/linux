#include <linux/module.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/mbus.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/scatterlist.h>
#include <linux/irq.h>
#include <linux/highmem.h>
#include <linux/sched.h>
#include <linux/mmc/host.h>

#include <asm/sizes.h>
#include <asm/unaligned.h>

#include <mach/hardware.h>
#include <mach/io.h>

#include <linux/mmc/arasan_plat.h>
#include "arasan.h"

static int detect = 0;
static int sdiocur = 0;
module_param(detect,int,S_IRUSR);
module_param(sdiocur ,int,S_IRUSR);

static void arasan_release(struct device *device)
{
	return;
}

static struct resource arasan0_resource[] = {
	[0] = {
		.name  = "sdio0_addr",
		.start = SDIO0_BASE,
		.end   = SDIO0_BASE + 0x100000 - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.name  = "sdio0_irq",
		.start = SDIO0_IRQ,
		.end   = SDIO0_IRQ,
		.flags = IORESOURCE_IRQ,
	}
};

void sdio0_powerup(void)
{
	int cur_bit = 0 << 6;
	switch(sdiocur)
	{
		case 2:cur_bit = 0 << 6;break;
		case 4:cur_bit = 1 << 6;break;
		case 8:cur_bit = 2 << 6;break;
		case 12:cur_bit = 3 << 6;break;
		default :cur_bit = 0 << 6;break;
	}
	gpio_write(GPIO_MUX1_EN | cur_bit, 43);  //sdio0_clk
	gpio_write(GPIO_MUX1_EN | cur_bit, 44);	//sdio0_cmd
	gpio_write(GPIO_MUX1_EN | cur_bit, 45);	//sdio0_data0
	gpio_write(GPIO_MUX1_EN | cur_bit, 46);	//sdio0_data1
	gpio_write(GPIO_MUX1_EN | cur_bit, 47);	//sdio0_data2
	gpio_write(GPIO_MUX1_EN | cur_bit, 48);	//sdio0_data3
	writel(0x80,IOMEM(IO_ADDRESS(0x100203e0)));

	if(detect) {
		gpio_write(GPIO_MUX1_EN | GPIO_DRIVE_8MA, 49);	//sdio0_detect
		gpio_write(GPIO_MUX1_EN | GPIO_DRIVE_8MA, 50);	//sdio0_pwsv
	} else {
		gpio_write(GPIO_DRIVE_8MA, 49);
		gpio_write(GPIO_OUT_EN | GPIO_OUT_LOW, 50);
	}

	gpio_write(GPIO_DRIVE_8MA, 51);	//sdio0_writeprotect
	gpio_write(GPIO_MUX1_EN | GPIO_DRIVE_8MA, 52);	//sdio0_lenon
}

struct arasan_platform_data arasan0_platform_data = {
	.need_poll = 0,
	.need_detect = 0,
	.use_pio = 0,
	.auto_cmd12 = 1,
	.card_irq = 0,
	.sdio_4bit_data = 1,
	.p_powerup = sdio0_powerup,
};

/* sdio 0 */
static struct platform_device arasan0_device = {
	.id = 0, 
	.name = ARASAN_DRIVER_NAME,
	.num_resources = ARRAY_SIZE(arasan0_resource),
	.resource = arasan0_resource,
	.dev = {
		.coherent_dma_mask = DMA_BIT_MASK(32),
		.platform_data = &arasan0_platform_data,
		.release = arasan_release,
	}    
};

static int __init arasan0_init(void)
{
	if(detect)
	{
		arasan0_platform_data.need_detect = 1;
	}
	platform_device_register(&arasan0_device);

	return 0;
}

static void __exit arasan0_exit(void)
{
	platform_device_unregister(&arasan0_device);
}

module_init(arasan0_init);
module_exit(arasan0_exit);

MODULE_LICENSE("GPL");

