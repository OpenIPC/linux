/*
 * Arasan MMC/SD/SDIO driver
 *
 *  This is the driver for the Arasan MMC/SD/SDIO host controller
 *  integrated in the STMicroelectronics platforms
 *
 * Author: Giuseppe Cavallaro <peppe.cavallaro@xxxxxx>
 * Copyright (C) 2010 STMicroelectronics Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

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

static int value = 0x54 ;
module_param(value,int,S_IRUSR);

static struct resource arasan2_resource[] = {
	[0] = {
		.name  = "sdio2_addr",
		.start = SDIO2_BASE,
		.end   = SDIO2_BASE + 0x10000 - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.name  = "sdio2_irq",
		.start = SDIO2_IRQ,
		.end   = SDIO2_IRQ,
		.flags = IORESOURCE_IRQ,
	}

};

struct resource *port0_r;
unsigned int port0_addr;

static void arasan_release(struct device *device)
{
	return;
}

#ifndef GPIO_BASE
	#define GPIO_BASE (0x10020000)
#endif
#ifndef GPIO6_MULT_USE_EN
	#define GPIO6_MULT_USE_EN (0x18)
#endif



void gpio6_reuse_sdio2(void)
{
	unsigned int gpio_6_reuse,gpio_6_reuse_value;
	void __iomem *gpio_6_resue_addr = NULL;
	int ret = 0; 

	gpio_6_reuse = GPIO_BASE+GPIO6_MULT_USE_EN;
	if (!request_mem_region(gpio_6_reuse,4,"gpio6_sdio2")) {
		pr_err("%s: ERROR: memory allocation failed"
				"cannot get the I/O addr 0x%x\n",
				__func__, (unsigned int)gpio_6_reuse);
		return;
	}
	gpio_6_resue_addr = ioremap(gpio_6_reuse,4);
	if (!gpio_6_resue_addr) {
		pr_err("%s: ERROR: memory mapping failed\n", __func__);
		ret = -ENOMEM;
		goto out_release_region;
	}
	/* ·ÖÅä¸øsdio2 */
	//gpio_6_reuse_value = readl(gpio_6_resue_addr);
	//gpio_6_reuse_value = readl(gpio_6_resue_addr);
	//gpio_6_reuse_value = 0xff;
	//gpio_6_reuse_value &= ~(0x5<<4); /* sdio1-cdtctæœªå¤ç”¨ï¼Œsd1-wptctä¸å¤ç”¨ */
	gpio_6_reuse_value = 0xf;
	writel(gpio_6_reuse_value, gpio_6_resue_addr);
	gpio_6_reuse_value = readl(gpio_6_resue_addr);
	//printk("value=%02x\n", gpio_6_reuse_value);

	iounmap(gpio_6_resue_addr);

out_release_region:
	release_mem_region(gpio_6_reuse, 4);
	return;

}

void gpio6_46_outlow(void)
{
	unsigned int gpio_6,gpio_6_value;
	void __iomem *gpio_6_addr = NULL;
	int ret = 0; 

	gpio_6 = GPIO_BASE+0xA0;
	if (!request_mem_region(gpio_6,12,"gpio6")) {
		pr_err("%s: ERROR: memory allocation failed"
				"cannot get the I/O addr 0x%x\n",
				__func__, (unsigned int)gpio_6);
		return;
	}
	gpio_6_addr = ioremap(gpio_6,12);
	if (!gpio_6_addr) {
		pr_err("%s: ERROR: memory mapping failed\n", __func__);
		ret = -ENOMEM;
		goto out_release_region;
	}

	/* gpio_6_4, gpio_6_6Êä³öÊ¹ÄÜ */
	gpio_6_value = readl(gpio_6_addr+4);
	gpio_6_value |= 0x5<<4;
	writel(gpio_6_value, gpio_6_addr+4);

	/* gpio_6_4µÍ, gpio_6_6Êä³ö¸ß */
	gpio_6_value = readl(gpio_6_addr);
	//gpio_6_value &= ~(0x5<<4);
	gpio_6_value &= ~(0x1<<6);
	//gpio_6_value &= ~(0x1<<4);
	writel(gpio_6_value, gpio_6_addr);
    mdelay(100);
	gpio_6_value |= 0x1<<6;
	writel(gpio_6_value, gpio_6_addr);
    mdelay(100);

	iounmap(gpio_6_addr);

out_release_region:
	release_mem_region(gpio_6, 12);
	return;
}
void gpio5_6_input(void)
{
	unsigned int gpio_5,gpio_5_value;
	void __iomem *gpio_5_addr = NULL;
	int ret = 0; 

	gpio_5 = GPIO_BASE+0x90;
	if (!request_mem_region(gpio_5,12,"gpio5")) {
		pr_err("%s: ERROR: memory allocation failed"
				"cannot get the I/O addr 0x%x\n",
				__func__, (unsigned int)gpio_5);
		return;
	}
	gpio_5_addr = ioremap(gpio_5,12);
	if (!gpio_5_addr) {
		pr_err("%s: ERROR: memory mapping failed\n", __func__);
		ret = -ENOMEM;
		goto out_release_region;
	}

	/* gpio_5_6ÊäÈëÊ¹ÄÜ */
    gpio_5_value = 0;
	gpio_5_value = readl(gpio_5_addr+4);
	gpio_5_value &= ~(0x1<<6);
	writel(gpio_5_value, gpio_5_addr+4);

	iounmap(gpio_5_addr);

out_release_region:
	release_mem_region(gpio_5, 12);
	return;
}


int wifi_powerup(unsigned int value)
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
	//ï¿½ï¿½ï¿½ï¿½
	gpio_dir_value = readl(IO_ADDRESS(GPIO_BASE + PortIndex*0x04));
	//printk("@@@@gpio_dir_value = %x@@@\n",gpio_dir_value);
	gpio_dir_value &= ~(1 << BitIndex | 1 << (8 +BitIndex) |1 << (16 + BitIndex));
	//printk("!!!!gpio_dir_value = %x!!! \n",gpio_dir_value);
	writel(gpio_dir_value,IO_ADDRESS(GPIO_BASE + PortIndex*0x04));
	//Ê¹ï¿½ï¿½
	gpio_bitdata_value = readl(IO_ADDRESS(GPIO_BASE + 0x44 +PortIndex*0x10));
	gpio_bitdata_value |=(1 << BitIndex); 
	writel(gpio_bitdata_value,IO_ADDRESS(GPIO_BASE + 0x44 +PortIndex*0x10));
	//ï¿½ï¿½ï¿½ï¿½//ï¿½ÈµÍºï¿½ï¿½
	gpio_value = readl(IO_ADDRESS(GPIO_BASE + 0x40 +PortIndex*0x10 ));
	gpio_value &=~(1 << BitIndex);
	writel(gpio_value,IO_ADDRESS(GPIO_BASE + 0x40 +PortIndex*0x10 ));
	mdelay(100);

	gpio_value = readl(IO_ADDRESS(GPIO_BASE + 0x40 +PortIndex*0x10 ));
	gpio_value |=(1 << BitIndex);
	writel(gpio_value,IO_ADDRESS(GPIO_BASE + 0x40 +PortIndex*0x10 ));
	mdelay(100);
	
	return 0;
}

/* GPIO¸´ÓÃµ½sdio2 */
void arasan2_powerup(void)
{
	gpio6_reuse_sdio2();
	mdelay(100);
	//gpio5_6_input();
    wifi_powerup(value);
}

struct arasan_platform_data arasan2_platform_data = {
	.need_poll = 0,
	.need_detect = 0,
	.use_pio = 0,
	.auto_cmd12 = 0,
	.card_irq = 1,
	.sdio_4bit_data = 1,

	.p_powerup = arasan2_powerup,
};
/* sdio 2 */
static struct platform_device arasan2_device = {
	//.id = -1,
	.name = ARASAN_DRIVER_NAME,
	.num_resources = ARRAY_SIZE(arasan2_resource),
	.resource = arasan2_resource,
	.dev = {
		.coherent_dma_mask = DMA_BIT_MASK(32),
		.platform_data = &arasan2_platform_data,
		.release = arasan_release,
	}
};

static int __init arasan2_init(void)
{
	platform_device_register(&arasan2_device);
	return 0;
}

static void __exit arasan2_exit(void)
{
	printk("arasan2_exit.\n");
	platform_device_unregister(&arasan2_device);
}

module_init(arasan2_init);
module_exit(arasan2_exit);

MODULE_LICENSE("GPL");
