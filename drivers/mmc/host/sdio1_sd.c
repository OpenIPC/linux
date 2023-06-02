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
#include <linux/mmc/arasan_plat.h>

#include <asm/sizes.h>
#include <asm/unaligned.h>

#include <mach/hardware.h>

#include "arasan.h"

static int detect = 0;
static unsigned int value = 0x14;
module_param(value,int,S_IRUSR);
module_param(detect,int,S_IRUSR);

static void arasan_release(struct device *device)
{
	return;
}

static struct resource arasan1_resource[] = {
	[0] = {
		.name  = "sdio1_addr",
		.start = SDIO1_BASE,
		.end   = SDIO1_BASE + 0x10000 - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.name  = "sdio1_irq",
		.start = SDIO1_IRQ,
		.end   = SDIO1_IRQ,
		.flags = IORESOURCE_IRQ,
	}

};
#ifndef GPIO_BASE
        #define GPIO_BASE (0x10020000)
#endif
#ifndef GPIO5_MULT_USE_EN
        #define GPIO5_MULT_USE_EN (0x14)
#endif




int GpioValueRead(unsigned int value)
{
	unsigned int PortIndex = (value&0xF0)>>4; 
	unsigned int BitIndex = value&0x0F;
	unsigned int gpio_dir_value = 0;
	unsigned int gpio_value = 0;
	unsigned int gpio_bitdata_value = 0;
	//printk("PortIndex = %x BitIndex = %x\n",PortIndex,BitIndex);
	if(PortIndex > 6)
	{
		printk("error : PortIndex = %d\n",PortIndex);
		return -1;
	}
	gpio_dir_value = readl(IO_ADDRESS(GPIO_BASE + PortIndex*0x04));
	//printk("@@@@gpio_dir_value = %x@@@\n",gpio_dir_value);
	gpio_dir_value &= ~(1 << BitIndex | 1 << (8 +BitIndex) |1 << (16 + BitIndex));
	//printk("!!!!gpio_dir_value = %x!!! \n",gpio_dir_value);
	writel(gpio_dir_value,IO_ADDRESS(GPIO_BASE + PortIndex*0x04));
	//----------------------------------------
	//使能
	gpio_bitdata_value = readl(IO_ADDRESS(GPIO_BASE + 0x44 +PortIndex*0x10));
	gpio_bitdata_value |=(1 << BitIndex); 
	writel(gpio_bitdata_value,IO_ADDRESS(GPIO_BASE + 0x44 +PortIndex*0x10));
	//配置
	gpio_value = readl(IO_ADDRESS(GPIO_BASE + 0x40 +PortIndex*0x10 ));
	gpio_value &=~(1 << BitIndex);
	writel(gpio_value,IO_ADDRESS(GPIO_BASE + 0x40 +PortIndex*0x10 ));
	return 0;
}


/* GPIO复用到sdio1 */
void arasan1_powerup(void)
{
	unsigned int gpio_5_reuse,gpio_5_reuse_value;
	void __iomem *gpio_5_resue_addr = NULL;
	int ret = 0; 


	gpio_5_reuse = GPIO_BASE+GPIO5_MULT_USE_EN;
	if (!request_mem_region(gpio_5_reuse,4,"gpio5_sdio1")) {
		pr_err("%s: ERROR: memory allocation failed"
				"cannot get the I/O addr 0x%x\n",
				__func__, (unsigned int)gpio_5_reuse);
		return;
	}
	gpio_5_resue_addr = ioremap(gpio_5_reuse,4);
	if (!gpio_5_resue_addr) {
		pr_err("%s: ERROR: memory mapping failed\n", __func__);
		ret = -ENOMEM;
		goto out_release_region;
	}
	/* 分配给sdio1 */                                                                            
	gpio_5_reuse_value = readl(gpio_5_resue_addr);
	gpio_5_reuse_value |= 0xff;
	if(detect)
	{
		gpio_5_reuse_value &= ~(0x1<<6); //open hot plug
		//printk("open hot plug\n");
	}
	else
	{
		gpio_5_reuse_value &= ~(0xf<<4); // no detect, no powersave, // no write protect, no ledo
		//printk("no hot plug\n");
   }                                 
	writel(gpio_5_reuse_value, gpio_5_resue_addr);
	//gpio_5_reuse_value = readl(gpio_5_resue_addr);

	iounmap(gpio_5_resue_addr);

out_release_region:
	release_mem_region(gpio_5_reuse, 4); 
	return;

}

struct arasan_platform_data arasan1_platform_data = {
	.need_poll = 0,
	.need_detect = 0,
	.use_pio = 0,
	.auto_cmd12 = 1,
	.card_irq = 0,
	.sdio_4bit_data = 1,

	.p_powerup = arasan1_powerup,
};

/* sdio 1 */
static struct platform_device arasan1_device = {
	.id = 1, 
	.name = ARASAN_DRIVER_NAME,
	.num_resources = ARRAY_SIZE(arasan1_resource),
	.resource = arasan1_resource,
	.dev = {
		.coherent_dma_mask = DMA_BIT_MASK(32),
		.platform_data = &arasan1_platform_data,
		.release = arasan_release,
	}    
};

static int __init arasan1_init(void)
{
	if(detect)
	{
		GpioValueRead(value);
		arasan1_platform_data.need_detect = 1;
	}
	platform_device_register(&arasan1_device);

	return 0;
}

static void __exit arasan1_exit(void)
{
	platform_device_unregister(&arasan1_device);
}

module_init(arasan1_init);
module_exit(arasan1_exit);

MODULE_LICENSE("GPL");

