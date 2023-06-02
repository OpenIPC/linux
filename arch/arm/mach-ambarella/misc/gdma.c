/*
 * arch/arm/plat-ambarella/generic/gdma.c
 *
 * Author: Louis Sun <lysun@ambarella.com>
 *
 * Copyright (C) 2004-2010, Ambarella, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <mach/hardware.h>
#include <mach/init.h>
#include <plat/iav_helper.h>
#include <plat/gdma.h>

#define TRANSFER_2D_WIDTH		(1 << 12 )		/* 4096 */
#define MAX_TRANSFER_2D_HEIGHT		(1 << 11 )		/* 2048 */
#define MAX_TRANSFER_SIZE_2D_UNIT	(TRANSFER_2D_WIDTH * MAX_TRANSFER_2D_HEIGHT)	/* 8MB */

#define TRANSFER_1D_WIDTH		TRANSFER_2D_WIDTH
#define MAX_TRANSFER_SIZE_1D_UNIT	TRANSFER_1D_WIDTH

/* transfer 6 big blocks (although maximum is 8), because we may do another 1 small block and 1 line. total 8 Ops */
#define MAX_TRANSFER_SIZE_ONCE		(MAX_TRANSFER_SIZE_2D_UNIT * 6)	/* 48 MB */
#define MAX_OPS				8

static struct completion	transfer_completion;
static struct mutex		transfer_mutex;


/* handle 8MB at one time */
static inline int transfer_big_unit(u8 *dest_addr, u8 *src_addr, u32 size)
{
	int row_count;
	if (size > MAX_TRANSFER_SIZE_2D_UNIT) {
		printk("transfer_unit size %d bigger than %d \n",
			size, MAX_TRANSFER_SIZE_2D_UNIT);
		return -1;
	}

	row_count = size / TRANSFER_2D_WIDTH;

	/* copy rows by 2D copy */
	if (row_count > 0) {
		amba_writel(GDMA_SRC_1_BASE_REG, (long)src_addr);
		amba_writel(GDMA_SRC_1_PITCH_REG, TRANSFER_2D_WIDTH);
		amba_writel(GDMA_DST_BASE_REG, (long)dest_addr);
		amba_writel(GDMA_DST_PITCH_REG, TRANSFER_2D_WIDTH);
		amba_writel(GDMA_WIDTH_REG, TRANSFER_2D_WIDTH - 1);
		amba_writel(GDMA_HEIGHT_REG, row_count - 1);
#if (GDMA_SUPPORT_ALPHA_BLEND == 1)
		amba_writel(GDMA_PIXELFORMAT_REG, 0x800);
		amba_writel(GDMA_ALPHA_REG, 0);
		amba_writel(GDMA_CLUT_BASE_REG, 0);
#endif

		/* start 2D copy */
		amba_writel(GDMA_OPCODE_REG, 1);
	}
	return 0;

}

/* use 1D copy to copy max  4KB each time */
static inline int transfer_small_unit(u8 *dest_addr, u8 *src_addr, u32 size)
{
	if (size > TRANSFER_1D_WIDTH) {
		printk("transfer_unit size %d bigger than %d \n",
			size, TRANSFER_1D_WIDTH);
		return -1;
	}

	/* linear copy */
	amba_writel(GDMA_SRC_1_BASE_REG, (long)src_addr);
	amba_writel(GDMA_DST_BASE_REG, (long)dest_addr);
	amba_writel(GDMA_WIDTH_REG, size - 1);
#if (GDMA_SUPPORT_ALPHA_BLEND == 1)
	amba_writel(GDMA_PIXELFORMAT_REG, 0x800);
	amba_writel(GDMA_ALPHA_REG, 0);
	amba_writel(GDMA_CLUT_BASE_REG, 0);
#endif

	/* start linear copy */
	amba_writel(GDMA_OPCODE_REG, 0);

	return 0;
}



/* this is async function, just fill dma registers and let it run*/
static inline int transfer_once(u8 *dest_addr, u8 *src_addr, u32 size)
{
	//total pending count must be no bigger than 8
	int big_count;
	int rows_count;
	int i;
	u32 transferred_bytes = 0;
	int remain_bytes ;

	if (size > MAX_TRANSFER_SIZE_ONCE)  {
		printk(" size too big %d for transfer once \n", size);
		return -1;
	}

	big_count = size/MAX_TRANSFER_SIZE_2D_UNIT;
	//big pages (each is 8MB)
	for (i = big_count ; i > 0; i--) {
		transfer_big_unit(dest_addr + transferred_bytes,
						src_addr  + transferred_bytes,
						MAX_TRANSFER_SIZE_2D_UNIT);
		transferred_bytes += MAX_TRANSFER_SIZE_2D_UNIT;
	}
	remain_bytes =  size - transferred_bytes;


	//transfer rows (align to TRANSFER_2D_WIDTH)
	rows_count = remain_bytes / TRANSFER_2D_WIDTH;
	if (rows_count > 0) {
		transfer_big_unit(dest_addr + transferred_bytes,
							src_addr  + transferred_bytes,
							TRANSFER_2D_WIDTH * rows_count);
		transferred_bytes += TRANSFER_2D_WIDTH * rows_count;
		remain_bytes =  size - transferred_bytes;
	}

	if (remain_bytes > 0) {
		transfer_small_unit(dest_addr + transferred_bytes,
						src_addr  + transferred_bytes, remain_bytes);
	}

	return 0;
}

/* this is synchronous function, will wait till transfer finishes */
int dma_memcpy(u8 *dest_addr, u8 *src_addr, u32 size)
{
	int remain_size = size;
	int transferred_size = 0;
	int current_transfer_size;

	if (size <= 0) {
		return -1;
	}

#if (GDMA_SUPPORT_ALPHA_BLEND == 1)
	if (size & 0x1) {
		printk("Size must be even !\n");
		return -1;
	}
#endif

	mutex_lock(&transfer_mutex);

	ambcache_clean_range((void *)ambarella_phys_to_virt((u32)src_addr), size);

	while (remain_size > 0)	{
		if (remain_size > MAX_TRANSFER_SIZE_ONCE) {
			remain_size -= MAX_TRANSFER_SIZE_ONCE;
			current_transfer_size = MAX_TRANSFER_SIZE_ONCE;
		} else {
			current_transfer_size = remain_size;
			remain_size = 0;
		}

		transfer_once(dest_addr + transferred_size,
			src_addr + transferred_size, current_transfer_size);
		wait_for_completion(&transfer_completion);
		transferred_size += current_transfer_size;
	}

	ambcache_inv_range((void *)ambarella_phys_to_virt((u32)dest_addr), size);

	mutex_unlock(&transfer_mutex);

	return 0;
}
EXPORT_SYMBOL(dma_memcpy);

static inline int transfer_pitch_unit(u8 *dest_addr, u8 *src_addr,u16 src_pitch, u16 dest_pitch, u16 width, u16 height)
{

	if (height <= 0) {
		return -1;
	}

	/* copy rows by 2D copy */
	while (height > MAX_TRANSFER_2D_HEIGHT) {
		amba_writel(GDMA_SRC_1_BASE_REG, (long)src_addr);
		amba_writel(GDMA_SRC_1_PITCH_REG, src_pitch);
		amba_writel(GDMA_DST_BASE_REG, (long)dest_addr);
		amba_writel(GDMA_DST_PITCH_REG, dest_pitch);
		amba_writel(GDMA_WIDTH_REG, width - 1);
		amba_writel(GDMA_HEIGHT_REG, MAX_TRANSFER_2D_HEIGHT - 1);
#if (GDMA_SUPPORT_ALPHA_BLEND == 1)
		amba_writel(GDMA_PIXELFORMAT_REG, 0x800);
		amba_writel(GDMA_ALPHA_REG, 0);
		amba_writel(GDMA_CLUT_BASE_REG, 0);
#endif

		/* start 2D copy */
		amba_writel(GDMA_OPCODE_REG, 1);
		height = height - MAX_TRANSFER_2D_HEIGHT;
		src_addr = src_addr + src_pitch * MAX_TRANSFER_2D_HEIGHT;
		dest_addr = dest_addr + dest_pitch * MAX_TRANSFER_2D_HEIGHT;
	}

		amba_writel(GDMA_SRC_1_BASE_REG, (long)src_addr);
		amba_writel(GDMA_SRC_1_PITCH_REG, src_pitch);
		amba_writel(GDMA_DST_BASE_REG, (long)dest_addr);
		amba_writel(GDMA_DST_PITCH_REG, dest_pitch);
		amba_writel(GDMA_WIDTH_REG, width - 1);
		amba_writel(GDMA_HEIGHT_REG, height - 1);
#if (GDMA_SUPPORT_ALPHA_BLEND == 1)
		amba_writel(GDMA_PIXELFORMAT_REG, 0x800);
		amba_writel(GDMA_ALPHA_REG, 0);
		amba_writel(GDMA_CLUT_BASE_REG, 0);
#endif

		/* start 2D copy */
		amba_writel(GDMA_OPCODE_REG, 1);

	return 0;

}

/* this is synchronous function, will wait till transfer finishes  width =< 4096 */
int dma_pitch_memcpy(struct gdma_param *params)
{
	int size = params->src_pitch * params->height;

	if (size <= 0 || params->src_pitch <= 0 || params->dest_pitch <= 0
		|| params->width > TRANSFER_2D_WIDTH) {
		printk(" invalid value \n");
		return -1;
	}

#if (GDMA_SUPPORT_ALPHA_BLEND == 1)
	if (size & 0x1) {
		printk("Size must be even !\n");
		return -1;
	}
#endif

	mutex_lock(&transfer_mutex);
	if (!params->src_non_cached) {
		ambcache_clean_range((void *)params->src_virt_addr, size);
	}
	transfer_pitch_unit((u8 *)params->dest_addr, (u8 *)params->src_addr,
		params->src_pitch, params->dest_pitch, params->width, params->height);

	wait_for_completion(&transfer_completion);

	if (!params->dest_non_cached) {
		ambcache_inv_range((void *)params->dest_virt_addr, size);
	}
	mutex_unlock(&transfer_mutex);

	return 0;
}

EXPORT_SYMBOL(dma_pitch_memcpy);

static irqreturn_t gdma_interrupt(int irq, void *dev_id)
{
	int pending_ops;
	pending_ops = amba_readl(GDMA_PENDING_OPS_REG);

	if (pending_ops == 0) {
		/* if no following transfer */
		complete(&transfer_completion);
	} else {

	}
	return IRQ_HANDLED;
}

static int hw_init(void)
{
	int	errorCode;
	/* request irq, no device id, no irq sharing */
	errorCode = request_irq(GDMA_IRQ, gdma_interrupt,
	IRQF_TRIGGER_RISING, "gdma", 0);

	if (errorCode) {
		printk("gdma irq request failed \n");
		return -1;
	}

	return 0;
}

/* wait till transmit completes */
static void wait_transmit_complete(void)
{
	int pending_ops;
	pending_ops = amba_readl(GDMA_PENDING_OPS_REG);

	while(pending_ops!= 0) {
		mdelay(10);
	}
}

static int __init gdma_init(void)
{
	/* hardware and irq init */
	if (hw_init() != 0)
		return -1;
	/* init completion */
	init_completion(&transfer_completion);
	mutex_init(&transfer_mutex);

	return 0;
}

static void __exit gdma_exit(void)
{
	wait_transmit_complete();
}

MODULE_AUTHOR("Louis Sun <lysun@ambarella.com>");
MODULE_DESCRIPTION("GDMA driver on Ambarella A5S / S2");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0");

module_init(gdma_init);
module_exit(gdma_exit);

