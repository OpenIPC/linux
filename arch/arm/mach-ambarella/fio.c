/*
 * arch/arm/plat-ambarella/generic/fio.c
 *
 * History:
 *	2008/03/05 - [Chien-Yang Chen] created file
 *	2008/01/09 - [Anthony Ginger] Port to 2.6.28.
 *
 * Copyright (C) 2004-2009, Ambarella, Inc.
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
#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <asm/setup.h>
#include <mach/hardware.h>
#include <asm/cacheflush.h>
#include <plat/fio.h>
#include <plat/sd.h>
#include <plat/nand.h>
#include <plat/rct.h>

static void *fio_4k_vaddr = NULL;

#if (FIO_INDEPENDENT_SD == 0)

static DECLARE_WAIT_QUEUE_HEAD(fio_wait);
static DEFINE_SPINLOCK(fio_lock);
static DEFINE_SPINLOCK(fio_sd0_int_lock);

static u32 fio_owner = SELECT_FIO_FREE;
static u32 fio_sd_owner_cnt = 0;
#if (CHIP_REV == A5S)
static u32 fio_default_owner = SELECT_FIO_SDIO;
#else
static u32 fio_default_owner = SELECT_FIO_SD;
#endif
static u32 fio_sd_int = 0;
static u32 fio_sdio_int = 0;


static int __init fio_default_owner_init(char *p)
{
	fio_default_owner = simple_strtoul(p, NULL, 0);
	return 0;
}
early_param("fio_default_owner", fio_default_owner_init);

void __fio_select_lock(int module)
{
	u32					fio_ctr;
	u32					fio_dmactr;
#if (CHIP_REV == A5S)
	unsigned long				flags;
#endif

	if (module == SELECT_FIO_FREE)
		return;

	fio_ctr = amba_readl(FIO_CTR_REG);
	fio_dmactr = amba_readl(FIO_DMACTR_REG);

	switch (module) {
	case SELECT_FIO_FL:
		fio_ctr &= ~FIO_CTR_XD;
		fio_dmactr = (fio_dmactr & 0xcfffffff) | FIO_DMACTR_FL;
		break;

	case SELECT_FIO_XD:
		fio_ctr |= FIO_CTR_XD;
		fio_dmactr = (fio_dmactr & 0xcfffffff) | FIO_DMACTR_XD;
		break;

	case SELECT_FIO_CF:
		fio_ctr &= ~FIO_CTR_XD;
		fio_dmactr = (fio_dmactr & 0xcfffffff) | FIO_DMACTR_CF;
		break;

	case SELECT_FIO_SD:
		fio_ctr &= ~FIO_CTR_XD;
		fio_dmactr = (fio_dmactr & 0xcfffffff) | FIO_DMACTR_SD;
		break;

	case SELECT_FIO_SDIO:
		fio_ctr |= FIO_CTR_XD;
		fio_dmactr = (fio_dmactr & 0xcfffffff) | FIO_DMACTR_SD;
		break;

	default:
		break;
	}

#if (CHIP_REV == A5S)
	spin_lock_irqsave(&fio_sd0_int_lock, flags);
	amba_clrbitsl(SD0_REG(SD_NISEN_OFFSET), SD_NISEN_CARD);
	spin_unlock_irqrestore(&fio_sd0_int_lock, flags);
#endif
	amba_writel(FIO_CTR_REG, fio_ctr);
	amba_writel(FIO_DMACTR_REG, fio_dmactr);
#if (CHIP_REV == A5S)
	if (module == SELECT_FIO_SD) {
		spin_lock_irqsave(&fio_sd0_int_lock, flags);
		amba_writel(SD0_REG(SD_NISEN_OFFSET), fio_sd_int);
		amba_writel(SD0_REG(SD_NIXEN_OFFSET), fio_sd_int);
		spin_unlock_irqrestore(&fio_sd0_int_lock, flags);
	} else if (module == SELECT_FIO_SDIO) {
		spin_lock_irqsave(&fio_sd0_int_lock, flags);
		amba_writel(SD0_REG(SD_NISEN_OFFSET), fio_sdio_int);
		amba_writel(SD0_REG(SD_NIXEN_OFFSET), fio_sdio_int);
		spin_unlock_irqrestore(&fio_sd0_int_lock, flags);
	}
#endif
}

static bool fio_check_free(u32 module)
{
	unsigned long flags;
	bool is_free = false;

	spin_lock_irqsave(&fio_lock, flags);

	if (fio_owner == SELECT_FIO_FREE) {
		is_free = true;
		fio_owner = module;
	} else if (fio_owner == module) {
		is_free = true;
		if (fio_owner != SELECT_FIO_SD)
			pr_warning("%s: module[%d] reentry!\n", __func__, module);
	}

	if (is_free && module == SELECT_FIO_SD)
		fio_sd_owner_cnt++;

	spin_unlock_irqrestore(&fio_lock, flags);

	return is_free;
}


void fio_select_lock(int module)
{
	wait_event(fio_wait, fio_check_free(module));
	__fio_select_lock(module);
}
EXPORT_SYMBOL(fio_select_lock);

void fio_unlock(int module)
{
	unsigned long flags;

	BUG_ON(fio_owner != module);

	spin_lock_irqsave(&fio_lock, flags);

	if (module != SELECT_FIO_SD || --fio_sd_owner_cnt == 0)
		fio_owner = SELECT_FIO_FREE;

	if (fio_owner == SELECT_FIO_FREE) {
		if (fio_default_owner != module)
			__fio_select_lock(fio_default_owner);
		wake_up(&fio_wait);
	}

	spin_unlock_irqrestore(&fio_lock, flags);
}
EXPORT_SYMBOL(fio_unlock);

static int fio_amb_sd0_is_enable(void)
{
	u32 fio_ctr;
	u32 fio_dmactr;

	fio_ctr = amba_readl(FIO_CTR_REG);
	fio_dmactr = amba_readl(FIO_DMACTR_REG);

	return (((fio_ctr & FIO_CTR_XD) == 0) &&
		((fio_dmactr & FIO_DMACTR_SD) == FIO_DMACTR_SD));
}

void fio_amb_sd0_set_int(u32 mask, u32 on)
{
	unsigned long				flags;
	u32					int_flag;

	spin_lock_irqsave(&fio_sd0_int_lock, flags);
	int_flag = amba_readl(SD0_REG(SD_NISEN_OFFSET));
	if (on)
		int_flag |= mask;
	else
		int_flag &= ~mask;
	fio_sd_int = int_flag;
	if (fio_amb_sd0_is_enable()) {
		amba_writel(SD0_REG(SD_NISEN_OFFSET), int_flag);
		amba_writel(SD0_REG(SD_NIXEN_OFFSET), int_flag);
	}
	spin_unlock_irqrestore(&fio_sd0_int_lock, flags);
}
EXPORT_SYMBOL(fio_amb_sd0_set_int);

static int fio_amb_sdio0_is_enable(void)
{
	u32 fio_ctr;
	u32 fio_dmactr;

	fio_ctr = amba_readl(FIO_CTR_REG);
	fio_dmactr = amba_readl(FIO_DMACTR_REG);

	return (((fio_ctr & FIO_CTR_XD) == FIO_CTR_XD) &&
		((fio_dmactr & FIO_DMACTR_SD) == FIO_DMACTR_SD));
}

void fio_amb_sdio0_set_int(u32 mask, u32 on)
{
	unsigned long				flags;
	u32					int_flag;

	spin_lock_irqsave(&fio_sd0_int_lock, flags);
	int_flag = amba_readl(SD0_REG(SD_NISEN_OFFSET));
	if (on)
		int_flag |= mask;
	else
		int_flag &= ~mask;
	fio_sdio_int = int_flag;
	if (fio_amb_sdio0_is_enable()) {
		amba_writel(SD0_REG(SD_NISEN_OFFSET), int_flag);
		amba_writel(SD0_REG(SD_NIXEN_OFFSET), int_flag);
	}
	spin_unlock_irqrestore(&fio_sd0_int_lock, flags);
}
EXPORT_SYMBOL(fio_amb_sdio0_set_int);
#endif

void ambarella_fio_prepare(void)
{
	fio_4k_vaddr = __arm_ioremap_exec(FIO_4K_PHYS_BASE, 0x1000, false);
	if (!fio_4k_vaddr)
		pr_warning("__arm_ioremap_exec for FIFO failed!\n");
}

void *ambarella_fio_push(void *func, u32 size)
{
	BUG_ON(!fio_4k_vaddr || (size > 0x1000));

	amba_writel(FIO_CTR_REG, (amba_readl(FIO_CTR_REG) | FIO_CTR_RR));
	memcpy(fio_4k_vaddr, func, size);

	flush_icache_range((unsigned long)(fio_4k_vaddr),
			(unsigned long)(fio_4k_vaddr) + (size));

	return fio_4k_vaddr;
}
EXPORT_SYMBOL(ambarella_fio_push);

void ambarella_fio_rct_reset(void)
{
	amba_rct_writel(FIO_RESET_REG, FIO_RESET_FIO_RST);
	mdelay(1);
	amba_rct_writel(FIO_RESET_REG, 0x0);
	mdelay(1);
}
EXPORT_SYMBOL(ambarella_fio_rct_reset);

/* ==========================================================================*/
int __init ambarella_init_fio(void)
{
	/* ioremap for self refresh */
	ambarella_fio_prepare();
#if defined(CONFIG_AMBARELLA_RAW_BOOT)
	amba_rct_writel(FIO_RESET_REG, (FIO_RESET_FIO_RST | FIO_RESET_CF_RST |
		FIO_RESET_XD_RST | FIO_RESET_FLASH_RST));
	mdelay(100);
	amba_rct_writel(FIO_RESET_REG, 0);
	mdelay(100);
	amba_clrbitsl(FIO_CTR_REG, FIO_CTR_RR);

	amba_setbitsl(NAND_CTR_REG, 0);
	amba_writel(NAND_CMD_REG, 0xff);
	mdelay(100);
	amba_writel(NAND_INT_REG, 0x0);
	amba_writel(NAND_CMD_REG, 0x90);
	mdelay(100);
	amba_writel(NAND_INT_REG, 0x0);
#endif
	return 0;
}

