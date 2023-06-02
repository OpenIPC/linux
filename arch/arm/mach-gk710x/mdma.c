/*!
*****************************************************************************
** \file        arch/arm/mach-gk7101/mdma.c
**
** \version
**
** \brief
**
** \attention   THIS SAMPLE CODE IS PROVIDED AS IS. GOKE MICROELECTRONICS
**              ACCEPTS NO RESPONSIBILITY OR LIABILITY FOR ANY ERRORS OR
**              OMMISSIONS
**
** (C) Copyright 2012-2015 by GOKE MICROELECTRONICS CO.,LTD
**
*****************************************************************************
*/
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/cpu.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/delay.h>

#include <mach/hardware.h>
#include <mach/io.h>
#include <plat/mdma.h>

#define TRANSFER_2D_WIDTH				(1<<12)		 /* 4096*/
#define MAX_TRANSFER_2D_HEIGHT		(1<<11)		/* 2048*/
#define MAX_TRANSFER_SIZE_2D_UNIT	(TRANSFER_2D_WIDTH * MAX_TRANSFER_2D_HEIGHT)	/* 8MB*/

#define TRANSFER_1D_WIDTH				TRANSFER_2D_WIDTH
#define MAX_TRANSFER_SIZE_1D_UNIT	TRANSFER_1D_WIDTH

/* transfer 6 big blocks (although maximum is 8), because we may do another 1 small block and 1 line. total 8 Ops */
#define MAX_TRANSFER_SIZE_ONCE		(MAX_TRANSFER_SIZE_2D_UNIT * 6)	/* 48 MB*/

#define MAX_OPS							8


static struct completion	transfer_completion;


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
		gk_dsp_writel(MDMA_SRC_1_BASE_REG, (long)src_addr);
		gk_dsp_writel(MDMA_SRC_1_PITCH_REG, TRANSFER_2D_WIDTH);
		gk_dsp_writel(MDMA_DST_BASE_REG, (long)dest_addr);
		gk_dsp_writel(MDMA_DST_PITCH_REG, TRANSFER_2D_WIDTH);
		gk_dsp_writel(MDMA_WIDTH_REG, TRANSFER_2D_WIDTH - 1);
		gk_dsp_writel(MDMA_HEIGHT_REG, row_count - 1);

		/* start 2D copy */
		gk_dsp_writel(MDMA_OPCODE_REG, 1);
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
	gk_dsp_writel(MDMA_SRC_1_BASE_REG, (long)src_addr);
	gk_dsp_writel(MDMA_DST_BASE_REG, (long)dest_addr);
	gk_dsp_writel(MDMA_WIDTH_REG, TRANSFER_1D_WIDTH - 1);

	/* start linear copy */
	gk_dsp_writel(MDMA_OPCODE_REG, 0);

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

	return 0;
}

EXPORT_SYMBOL(dma_memcpy);


static irqreturn_t mdma_interrupt(int irq, void *dev_id)
{
	int pending_ops;
	pending_ops = gk_dsp_readl(MDMA_PENDING_OPS_REG);

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
	errorCode = request_irq(MDMA_IRQ, mdma_interrupt, IRQF_TRIGGER_RISING, "mdma", 0);

	if (errorCode) {
		printk("mdma irq request failed \n");
		return -1;
	}
	printk("mdma request irq: %d \n", MDMA_IRQ);

	return 0;
}


/* wait till transmit completes */
static void wait_transmit_complete(void)
{
	int pending_ops;
	pending_ops = gk_dsp_readl(MDMA_PENDING_OPS_REG);

	while(pending_ops!= 0) {
		mdelay(10);
	}
}

static int __init mdma_init(void)
{
	printk("mdma init...\n");

	/* hardware and irq init */
	if (hw_init() != 0)
		return -1;
	/* init completion */
	init_completion(&transfer_completion);

	return 0;
}


static void __exit mdma_exit(void)
{
	wait_transmit_complete();
}

MODULE_AUTHOR("Louis ");
MODULE_DESCRIPTION("MDMA driver on gk");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0");

module_init(mdma_init);
module_exit(mdma_exit);

