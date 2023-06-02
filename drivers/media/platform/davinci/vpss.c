/*
 * Copyright (C) 2009 Texas Instruments.
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * common vpss driver for all video drivers.
 */
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <linux/compiler.h>
#include <linux/io.h>
#include <mach/hardware.h>
#include <media/davinci/vpss.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("VPSS Driver");
MODULE_AUTHOR("Texas Instruments");

/* DM644x defines */
#define DM644X_SBL_PCR_VPSS		(4)

/* vpss BL register offsets */
#define DM355_VPSSBL_CCDCMUX		0x1c
/* vpss CLK register offsets */
#define DM355_VPSSCLK_CLKCTRL		0x4
#define DM355_VPSSBL_PCR		0x4
#define DM355_VPSSBL_MEMCTRL		0x18

/* masks and shifts */
#define DM365_PCCR			0x04
#define DM365_ISP_REG_BASE		0x01c70000
#define DM365_VPSS_REG_BASE		0x01c70200
#define DM365_VPBE_CLK_CTRL		0x00
#define DM365_ISP5_CCDCMUX		0x20
#define DM365_ISP5_PG_FRAME_SIZE 	0x28
#define DM365_CCDC_PG_VD_POL_SHIFT 	0
#define DM365_CCDC_PG_HD_POL_SHIFT 	1
#define DM365_VPSS_INTSTAT		0x0C
#define DM365_VPSS_INTSEL1		0x10


#define VPSS_CLK_CTRL			0x01C40044
#define CCD_SRC_SEL_MASK		(BIT_MASK(5) | BIT_MASK(4))
#define CCD_SRC_SEL_SHIFT		4
#define CCD_INT_SEL_MASK		(BIT_MASK(12) | BIT_MASK(11)|\
					BIT_MASK(10) | BIT_MASK(9)  |\
					BIT_MASK(8)  | BIT_MASK(4)  |\
					BIT_MASK(3)  | BIT_MASK(2)  |\
					BIT_MASK(1)  | BIT_MASK(0))

#define VPSS_HSSISEL_SHIFT		4


/**
 * vpss operations. Depends on platform. Not all functions are available
 * on all platforms. The api, first check if a function is available before
 * invoking it. In the probe, the function ptrs are intialized based on
 * vpss name. vpss name can be "dm355_vpss", "dm644x_vpss" etc.
 */
struct vpss_hw_ops {
	/* enable clock */
	int (*enable_clock)(enum vpss_clock_sel clock_sel, int en);
	/* select input to ccdc */
	void (*select_ccdc_source)(enum vpss_ccdc_source_sel src_sel);
	/* clear wbl overflow bit */
	int (*clear_wbl_overflow)(enum vpss_wbl_sel wbl_sel);
	/* set sync polarity */
	void (*set_sync_pol)(struct vpss_sync_pol);
	/* set the PG_FRAME_SIZE register*/
	void (*set_pg_frame_size)(struct vpss_pg_frame_size);
	/* check and clear interrupt if occured */
	int (*dma_complete_interrupt)(void);
};

/* vpss configuration */
struct vpss_oper_config {
	__iomem void *vpss_regs_base0;
	__iomem void *vpss_regs_base1;
	resource_size_t *vpss_regs_base2;
	struct resource		*r1;
	resource_size_t		len1;
	struct resource		*r2;
	resource_size_t		len2;
	char vpss_name[32];
	spinlock_t vpss_lock;
	struct vpss_hw_ops hw_ops;
};

static struct vpss_oper_config oper_cfg;

/* register access routines */
static inline u32 bl_regr(u32 offset)
{
	return __raw_readl(oper_cfg.vpss_regs_base0 + offset);
}

static inline void bl_regw(u32 val, u32 offset)
{
	u32 read_value;

	__raw_writel(val, oper_cfg.vpss_regs_base0 + offset);

	read_value = __raw_readl(oper_cfg.vpss_regs_base0 + offset);
}

static inline u32 isp5_read(u32 offset)
{
	return __raw_readl(oper_cfg.vpss_regs_base0 + offset);
}

static inline void isp5_write(u32 val, u32 offset)
{
	u32 read_value;

	__raw_writel(val, oper_cfg.vpss_regs_base0 + offset);

	read_value = __raw_readl(oper_cfg.vpss_regs_base0 + offset);
}

static inline u32 vpss_regr(u32 offset)
{
	return __raw_readl(oper_cfg.vpss_regs_base1 + offset);
}

static inline void vpss_regw(u32 val, u32 offset)
{
	u32 read_value;

	__raw_writel(val, oper_cfg.vpss_regs_base1 + offset);

	read_value = __raw_readl(oper_cfg.vpss_regs_base1 + offset);

}

static void dm355_select_ccdc_source(enum vpss_ccdc_source_sel src_sel)
{
	bl_regw(src_sel << VPSS_HSSISEL_SHIFT, DM355_VPSSBL_CCDCMUX);
}

static void dm365_select_ccdc_source(enum vpss_ccdc_source_sel src_sel)
{
	u32 temp = isp5_read(DM365_ISP5_CCDCMUX) & ~CCD_SRC_SEL_MASK;

	/* if we are using pattern generator, enable it */
	if (src_sel == VPSS_PGLPBK || src_sel == VPSS_CCDCPG)
		temp |= 0x08;

	temp |= (src_sel << CCD_SRC_SEL_SHIFT);
	isp5_write(temp, DM365_ISP5_CCDCMUX);
}

/**
 *  dm365_dma_complete_interrupt - check and clear RSZ_INT_DMA
 *
 *  This is called to update check and clear RSZ_INT_DMA interrupt
 */
static int dm365_dma_complete_interrupt(void)
{
	u32 status;

	status = isp5_read(DM365_VPSS_INTSTAT);
	/* Check and clear bit 15 */
	if (status & 0x8000) {
		status &= 0x8000;
		isp5_write(status, DM365_VPSS_INTSTAT);
		/* clear the interrupt */
		return 0;
	}
	return 1;
}

int vpss_dma_complete_interrupt(void)
{
	if (!oper_cfg.hw_ops.dma_complete_interrupt)
		return 2;
	return oper_cfg.hw_ops.dma_complete_interrupt();

}
EXPORT_SYMBOL(vpss_dma_complete_interrupt);

int vpss_select_ccdc_source(enum vpss_ccdc_source_sel src_sel)
{
	if (!oper_cfg.hw_ops.select_ccdc_source)
		return -1;

	oper_cfg.hw_ops.select_ccdc_source(src_sel);

	return 0;
}
EXPORT_SYMBOL(vpss_select_ccdc_source);

static int dm644x_clear_wbl_overflow(enum vpss_wbl_sel wbl_sel)
{
	u32 mask = 1, val;

	if (wbl_sel < VPSS_PCR_AEW_WBL_0 ||
	    wbl_sel > VPSS_PCR_CCDC_WBL_O)
		return -1;

	/* writing a 0 clear the overflow */
	mask = ~(mask << wbl_sel);
	val = bl_regr(DM644X_SBL_PCR_VPSS) & mask;
	bl_regw(val, DM644X_SBL_PCR_VPSS);

	return 0;
}

/**
 *  vpss_pcr_control - common function for updating vpsssbl pcr register
 *  @val: value to be written
 *  @mask: bit mask
 *  @shift: shift for mask and val
 *
 *  This is called to update VPSSSBL PCR register
 */
static void vpss_pcr_control(int val, int mask, int shift)
{
	unsigned long flags;
	u32 utemp;

	spin_lock_irqsave(&oper_cfg.vpss_lock, flags);

	utemp = bl_regr(DM355_VPSSBL_PCR);
	mask <<= shift;
	val <<= shift;
	utemp &= (~mask);
	utemp |= val;

	bl_regw(utemp, DM355_VPSSBL_PCR);
	spin_unlock_irqrestore(&oper_cfg.vpss_lock, flags);
}

/**
 *  vpss_dm355_assign_wblctrl_master - select WBLCTRL/DDR2 read master
 *  @master: memory master
 *
 *  This is called to assign DDR2/WBLCTRL master. Use this in only DM355
 */
void vpss_dm355_assign_wblctrl_master(enum dm355_wblctrl master)
{
	/* WBLCTRL is bit 6 */
	vpss_pcr_control(master, 1, 6);
}
EXPORT_SYMBOL(vpss_dm355_assign_wblctrl_master);

/**
 *  vpss_dm355_assign_rblctrl_master - select RBLCTRL/DDR2 read master
 *  @master: memory master
 *
 *  This is called to assign DDR2/RBLCTRL master. Use this in only DM355
 */
void vpss_dm355_assign_rblctrl_master(enum dm355_rblctrl master)
{
	/* RBLCTRL is bit 5 & 4 */
	vpss_pcr_control(master, 3, 5);
}
EXPORT_SYMBOL(vpss_dm355_assign_rblctrl_master);

/**
 *  vpss_memory_control - common function for updating memory
 *  control register
 *  @en: enable/disable
 *  @mask: bit mask
 *
 *  This is called to update memory control register
 */
static void vpss_mem_control(int en, u32 mask)
{
	unsigned long flags;
	u32 utemp;

	spin_lock_irqsave(&oper_cfg.vpss_lock, flags);
	utemp = bl_regr(DM355_VPSSBL_MEMCTRL);
	if (en)
		utemp |= mask;
	else
		utemp &= (~mask);

	bl_regw(utemp, DM355_VPSSBL_MEMCTRL);
	spin_unlock_irqrestore(&oper_cfg.vpss_lock, flags);
}

/**
 *  vpss_dm355_assign_dfc_memory_master - select dfc memory by IPIPE/CCDC
 *  @master: memory master
 *
 *  This is called to assign dfc memory mastership to IPIPE or CCDC.
 *  Use this in only DM355
 */
void vpss_dm355_assign_dfc_memory_master(enum dm355_dfc_mem_sel master)
{
	if (master == DM355_DFC_MEM_IPIPE)
		vpss_mem_control(0, 0x1);
	else
		vpss_mem_control(1, 0x1);
}
EXPORT_SYMBOL(vpss_dm355_assign_dfc_memory_master);

/**
 *  vpss_dm355_ipipe_enable_any_address - IPIPE can use any address type
 *  @en: enable/disable non-aligned buffer address use.
 *
 *  This is called to allow IPIPE to use non-aligned buffer address.
 *  Applicable only to DM355.
 */
void vpss_dm355_ipipe_enable_any_address(int en)
{
	if (en)
		vpss_mem_control(1, 0x4);
	else
		vpss_mem_control(0, 0x4);
}
EXPORT_SYMBOL(vpss_dm355_ipipe_enable_any_address);

/*
 *  vpss_dm355_assign_int_memory_master - assign internal module memory
 *  @master: master for internal memory
 *
 *  This function will select the module that gets access to internal memory.
 *  Choice is either IPIPE or CFALD. Applicable only on DM355
 */
void vpss_dm355_assign_int_memory_master(enum dm355_int_mem_sel master)
{
	if (master == DM355_INT_MEM_IPIPE)
		vpss_mem_control(0, 0x2);
	else
		vpss_mem_control(1, 0x2);

}
EXPORT_SYMBOL(vpss_dm355_assign_int_memory_master);

#if 0
static void dm365_enable_irq(void)
{
	u32 current_val = isp5_read(DM365_VPSS_INTSEL1);
	/*just enable INTSEL0 and INTSEL1 and leave everything else as is*/
	current_val &= ~(CCD_INT_SEL_MASK);
	current_val |= BIT_MASK(8);
	isp5_write(current_val, DM365_VPSS_INTSEL1);
}
#endif

static void dm365_set_sync_pol(struct vpss_sync_pol sync)
{
	int val = 0;
	val = isp5_read(DM365_ISP5_CCDCMUX);

	val |= (sync.ccdpg_hdpol << DM365_CCDC_PG_HD_POL_SHIFT);
	val |= (sync.ccdpg_vdpol << DM365_CCDC_PG_VD_POL_SHIFT);

	isp5_write(val, DM365_ISP5_CCDCMUX);
}

void vpss_set_sync_pol(struct vpss_sync_pol sync)
{
	if (!oper_cfg.hw_ops.set_sync_pol)
		return;

	oper_cfg.hw_ops.set_sync_pol(sync);
}
EXPORT_SYMBOL(vpss_set_sync_pol);

static void dm365_set_pg_frame_size(struct vpss_pg_frame_size frame_size)
{
	int current_reg = ((frame_size.hlpfr >> 1) - 1) << 16;

	current_reg |= (frame_size.pplen - 1);
	isp5_write(current_reg, DM365_ISP5_PG_FRAME_SIZE);
}

void vpss_set_pg_frame_size(struct vpss_pg_frame_size frame_size)
{
	if (!oper_cfg.hw_ops.set_pg_frame_size)
		return;

	oper_cfg.hw_ops.set_pg_frame_size(frame_size);
}
EXPORT_SYMBOL(vpss_set_pg_frame_size);

int vpss_clear_wbl_overflow(enum vpss_wbl_sel wbl_sel)
{
	if (!oper_cfg.hw_ops.clear_wbl_overflow)
		return -1;

	return oper_cfg.hw_ops.clear_wbl_overflow(wbl_sel);
}
EXPORT_SYMBOL(vpss_clear_wbl_overflow);

/**
 *  dm355_enable_clock - Enable VPSS Clock
 *  @clock_sel: CLock to be enabled/disabled
 *  @en: enable/disable flag
 *
 *  This is called to enable or disable a vpss clock
 */
static int dm355_enable_clock(enum vpss_clock_sel clock_sel, int en)
{
	unsigned long flags;
	u32 utemp, mask = 0x1, shift = 0;

	switch (clock_sel) {
	case VPSS_VPBE_CLOCK:
		/* nothing since lsb */
		break;
	case VPSS_VENC_CLOCK_SEL:
		shift = 2;
		break;
	case VPSS_CFALD_CLOCK:
		shift = 3;
		break;
	case VPSS_H3A_CLOCK:
		shift = 4;
		break;
	case VPSS_IPIPE_CLOCK:
		shift = 5;
		break;
	case VPSS_CCDC_CLOCK:
		shift = 6;
		break;
	default:
		printk(KERN_ERR "dm355_enable_clock:"
				" Invalid selector: %d\n", clock_sel);
		return -1;
	}

	spin_lock_irqsave(&oper_cfg.vpss_lock, flags);
	utemp = vpss_regr(DM355_VPSSCLK_CLKCTRL);
	if (!en)
		utemp &= ~(mask << shift);
	else
		utemp |= (mask << shift);

	vpss_regw(utemp, DM355_VPSSCLK_CLKCTRL);
	spin_unlock_irqrestore(&oper_cfg.vpss_lock, flags);

	return 0;
}

static int dm365_enable_clock(enum vpss_clock_sel clock_sel, int en)
{
	unsigned long flags;
	u32 utemp, mask = 0x1, shift = 0, offset = DM365_PCCR;
	u32 (*read)(u32 offset) = isp5_read;
	void(*write)(u32 val, u32 offset) = isp5_write;

	switch (clock_sel) {
	case VPSS_BL_CLOCK:
		break;
	case VPSS_CCDC_CLOCK:
		shift = 1;
		break;
	case VPSS_H3A_CLOCK:
		shift = 2;
		break;
	case VPSS_RSZ_CLOCK:
		shift = 3;
		break;
	case VPSS_IPIPE_CLOCK:
		shift = 4;
		break;
	case VPSS_IPIPEIF_CLOCK:
		shift = 5;
		break;
	case VPSS_PCLK_INTERNAL:
		shift = 6;
		break;
	case VPSS_PSYNC_CLOCK_SEL:
		shift = 7;
		break;
	case VPSS_VPBE_CLOCK:
		read = vpss_regr;
		write = vpss_regw;
		offset = DM365_VPBE_CLK_CTRL;
		break;
	case VPSS_VENC_CLOCK_SEL:
		shift = 2;
		read = vpss_regr;
		write = vpss_regw;
		offset = DM365_VPBE_CLK_CTRL;
		break;
	case VPSS_LDC_CLOCK:
		shift = 3;
		read = vpss_regr;
		write = vpss_regw;
		offset = DM365_VPBE_CLK_CTRL;
		break;
	case VPSS_FDIF_CLOCK:
		shift = 4;
		read = vpss_regr;
		write = vpss_regw;
		offset = DM365_VPBE_CLK_CTRL;
		break;
	case VPSS_OSD_CLOCK_SEL:
		shift = 6;
		read = vpss_regr;
		write = vpss_regw;
		offset = DM365_VPBE_CLK_CTRL;
		break;
	case VPSS_LDC_CLOCK_SEL:
		shift = 7;
		read = vpss_regr;
		write = vpss_regw;
		offset = DM365_VPBE_CLK_CTRL;
		break;
	default:
		printk(KERN_ERR "dm365_enable_clock: Invalid selector: %d\n",
		       clock_sel);
		return -1;
	}

	spin_lock_irqsave(&oper_cfg.vpss_lock, flags);
	utemp = read(offset);
	if (!en) {
		mask = ~mask;
		utemp &= (mask << shift);
	} else
		utemp |= (mask << shift);

	write(utemp, offset);
	spin_unlock_irqrestore(&oper_cfg.vpss_lock, flags);

	return 0;
}

int vpss_enable_clock(enum vpss_clock_sel clock_sel, int en)
{
	if (!oper_cfg.hw_ops.enable_clock)
		return -1;

	return oper_cfg.hw_ops.enable_clock(clock_sel, en);
}
EXPORT_SYMBOL(vpss_enable_clock);

static int vpss_probe(struct platform_device *pdev)
{
	int status, dm355 = 0, dm365 = 0;

	if (!pdev->dev.platform_data) {
		dev_err(&pdev->dev, "no platform data\n");
		return -ENOENT;
	}
	strcpy(oper_cfg.vpss_name, pdev->dev.platform_data);

	if (!strcmp(oper_cfg.vpss_name, "dm355_vpss"))
		dm355 = 1;
	else if (!strcmp(oper_cfg.vpss_name, "dm365_vpss"))
		dm365 = 1;
	else if (strcmp(oper_cfg.vpss_name, "dm644x_vpss")) {
		dev_err(&pdev->dev, "vpss driver not supported on"
			" this platform\n");
		return -ENODEV;
	}

	dev_info(&pdev->dev, "%s vpss probed\n", oper_cfg.vpss_name);
	oper_cfg.r1 = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!oper_cfg.r1)
		return -ENOENT;

	oper_cfg.len1 = oper_cfg.r1->end - oper_cfg.r1->start + 1;

	oper_cfg.r1 = request_mem_region(oper_cfg.r1->start, oper_cfg.len1,
					 oper_cfg.r1->name);
	if (!oper_cfg.r1)
		return -EBUSY;

	oper_cfg.vpss_regs_base0 = ioremap(oper_cfg.r1->start, oper_cfg.len1);
	if (!oper_cfg.vpss_regs_base0) {
		status = -EBUSY;
		goto fail1;
	}

	if (dm355 || dm365) {
		oper_cfg.r2 = platform_get_resource(pdev, IORESOURCE_MEM, 1);
		if (!oper_cfg.r2) {
			status = -ENOENT;
			goto fail2;
		}
		oper_cfg.len2 = oper_cfg.r2->end - oper_cfg.r2->start + 1;
		oper_cfg.r2 = request_mem_region(oper_cfg.r2->start,
						 oper_cfg.len2,
						 oper_cfg.r2->name);
		if (!oper_cfg.r2) {
			status = -EBUSY;
			goto fail2;
		}

		oper_cfg.vpss_regs_base1 = ioremap(oper_cfg.r2->start,
						  oper_cfg.len2);
		if (!oper_cfg.vpss_regs_base1) {
			status = -EBUSY;
			goto fail3;
		}
	}

	if (dm355) {
		oper_cfg.hw_ops.enable_clock = dm355_enable_clock;
		oper_cfg.hw_ops.select_ccdc_source = dm355_select_ccdc_source;
	} else if (dm365) {
		oper_cfg.hw_ops.enable_clock = dm365_enable_clock;
		oper_cfg.hw_ops.select_ccdc_source = dm365_select_ccdc_source;
		oper_cfg.hw_ops.set_sync_pol = dm365_set_sync_pol;
		oper_cfg.hw_ops.set_pg_frame_size = dm365_set_pg_frame_size;
		oper_cfg.hw_ops.dma_complete_interrupt =
				dm365_dma_complete_interrupt;

	} else if (!strcmp(oper_cfg.vpss_name, "dm644x_vpss"))
		oper_cfg.hw_ops.clear_wbl_overflow = dm644x_clear_wbl_overflow;

	if (dm355) {
		/*
		 * These values being written to INTSEL and EVTSEL
		 * registers match those in LSP 2.10
		 */
		bl_regw(0xff83ff10, 0x10);
		bl_regw(0x7b3c0004, 0x14);
	}
	if (dm365) {
		/**
		 * These values being written to INTSEL and EVTSEL
		 * registers match those in LSP 2.10
		 */
		isp5_write((isp5_read(0x4) | 0x0000007f), 0x4);
		isp5_write((isp5_read(0x8) | 0x00000002), 0x8);
		/* INT0, INT1, AF */
		isp5_write((isp5_read(0x10) | 0x0b1f0100), 0x10);
		/* AEW, RSZ_INT_DMA */
#ifdef CONFIG_VIDEO_YCBCR
		isp5_write((isp5_read(0x14) | 0x1f0a160d), 0x14);
#else
		isp5_write((isp5_read(0x14) | 0x1f0a0f0d), 0x14);
#endif
		/* VENC */
		isp5_write((isp5_read(0x18) | 0x00000015), 0x18);
		/* No event selected */
		isp5_write((isp5_read(0x1c) | 0x00000000), 0x1c);
	}

	spin_lock_init(&oper_cfg.vpss_lock);
	dev_info(&pdev->dev, "%s vpss probe success\n", oper_cfg.vpss_name);
	return 0;

fail3:
	release_mem_region(oper_cfg.r2->start, oper_cfg.len2);
fail2:
	iounmap(oper_cfg.vpss_regs_base0);
fail1:
	release_mem_region(oper_cfg.r1->start, oper_cfg.len1);
	return status;
}

static int vpss_remove(struct platform_device *pdev)
{
	iounmap(oper_cfg.vpss_regs_base0);
	release_mem_region(oper_cfg.r1->start, oper_cfg.len1);
	if (!strcmp(oper_cfg.vpss_name, "dm355_vpss") ||
	    !strcmp(oper_cfg.vpss_name, "dm365_vpss")) {
		iounmap(oper_cfg.vpss_regs_base1);
		release_mem_region(oper_cfg.r2->start, oper_cfg.len2);
	}

	return 0;
}

static struct platform_driver vpss_driver = {
	.driver = {
		.name	= "vpss",
		.owner = THIS_MODULE,
	},
	.remove = vpss_remove,
	.probe = vpss_probe,
};

static void vpss_exit(void)
{
	iounmap(oper_cfg.vpss_regs_base2);
	release_mem_region(*oper_cfg.vpss_regs_base2, 4);
	platform_driver_unregister(&vpss_driver);
}

static int __init vpss_init(void)
{
	if (request_mem_region(VPSS_CLK_CTRL, 4, "vpss_clock_control")) {
		oper_cfg.vpss_regs_base2 = ioremap(VPSS_CLK_CTRL, 4);
		__raw_writel(0x18, oper_cfg.vpss_regs_base2);
	} else
		return -EBUSY;

	return platform_driver_register(&vpss_driver);
}
subsys_initcall(vpss_init);
module_exit(vpss_exit);
