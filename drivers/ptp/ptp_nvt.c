/**
    NVT ptp function
    NVT ptp driver
    @file       ptp_nvt.c
    @ingroup
    @note
    Copyright   Novatek Microelectronics Corp. 2021.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.
*/
#include <linux/err.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/mod_devicetable.h>
#include <linux/platform_device.h>
#include <linux/ptp_clock_kernel.h>
#include <linux/types.h>
#include <linux/clocksource.h>
#include <linux/time.h>
#include <linux/timecounter.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/irqreturn.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/workqueue.h>

#define PTP_MAX_VALUE 0xFFFFFFFF
#define PTP_SHIFT	32ULL
#define PTP_MAX_HZ 3000000
#define PTP_MAX_DIV_BIT 255
#define PTP_TIMER_NUM (18)
#define PTP_TIMER_DIV_SRC 1
#if  (PTP_TIMER_DIV_SRC == 0)
	#define PTP_TIMER_DIV_DEF 0xFFFFFF00
#else
	#define PTP_TIMER_DIV_DEF 0xFFFF00FF
#endif
#define PTP_TIMER_DIV_BASE 0x30
#define PTP_TIMER_DIV_VAL (1 << ( 16 + PTP_TIMER_DIV_SRC) )
#define PTP_TIMER_IRQ_ENABLE 0x00
#define PTP_TIMER_IRQ_STS 0x10
#define PTP_TIMER_IRQ_BASE 0x20
#define PTP_TIMER_IRQ_VAL (1 << PTP_TIMER_NUM)
#define PTP_TIMER_CTL_BASE (0x10 * PTP_TIMER_NUM + 0x100)
#define PTP_TIMER_TVAL_BASE (PTP_TIMER_CTL_BASE + 0x04)
#define PTP_TIMER_VALUE_BASE (PTP_TIMER_CTL_BASE + 0x08)
#define PTP_TIMER_CNT_BASE (PTP_TIMER_CTL_BASE + 0x0C)
#define PTP_TIMER_ENABLE 1
#define PTP_TIMER_DISABLE 0
#define PTP_TIMER_FREQ_SHIFT (PTP_TIMER_DIV_SRC * 8)
#define PTP_TIMER_UP (0x3 + 0x4 * PTP_TIMER_DIV_SRC)
/* ptp dte priv structure */
struct ptp_dte {
	void __iomem *regs;
	struct ptp_clock *ptp_clk;
	struct ptp_clock_info caps;
	struct device *dev;
	spinlock_t lock;

	struct cyclecounter cc;
	struct timecounter tc;
	struct clk *clk;
	u32 freq;
};

struct ptp_dte *ptp_dte;
static int ptp_cyclecounter_init(struct ptp_dte *ptp);
static int ptp_dte_enable(struct ptp_clock_info *ptp,
			    struct ptp_clock_request *rq, int on);

#define MAX_TIMER_NUM 20
#ifdef CONFIG_NOVATEK_TIMER
extern void __iomem *nvttmr_base;
extern spinlock_t nvttmr_lock;
extern u32 nvttmr_in_use;
#else
void __iomem *nvttmr_base;
spinlock_t nvttmr_lock;
u32 nvttmr_in_use;
#endif

static s64 dte_read_nco(void __iomem *regs)
{
	u32 sum1;
	s64 ns;
	unsigned long flags;

	spin_lock_irqsave(&nvttmr_lock, flags);

	sum1 = readl(regs + PTP_TIMER_VALUE_BASE);

	ns = (s64)sum1;

	spin_unlock_irqrestore(&nvttmr_lock, flags);
	return ns;
}

static int ptp_dte_adjfreq(struct ptp_clock_info *ptp, s32 ppb)
{
	unsigned long flags;
	struct ptp_dte *ptp_dte = container_of(ptp, struct ptp_dte, caps);
	
	
	dev_err(ptp_dte->dev, "%s not support\n", __func__);
	return -EINVAL;

	if (abs(ppb) > ptp_dte->caps.max_adj || ppb < 0) {
		dev_err(ptp_dte->dev, "ppb adj is wrong value\n");
		return -EINVAL;
	}

	ptp_dte_enable(&ptp_dte->caps ,0,PTP_TIMER_DISABLE);

	spin_lock_irqsave(&ptp_dte->lock, flags);
	ptp_dte->freq = ppb;
	spin_unlock_irqrestore(&ptp_dte->lock, flags);

	ptp_dte_enable(&ptp_dte->caps ,0,PTP_TIMER_ENABLE);
	ptp_cyclecounter_init(ptp_dte);
	return 0;
}

static int ptp_dte_adjtime(struct ptp_clock_info *ptp, s64 delta)
{
	unsigned long flags;
	struct ptp_dte *ptp_dte = container_of(ptp, struct ptp_dte, caps);

	spin_lock_irqsave(&ptp_dte->lock, flags);

	timecounter_adjtime(&ptp_dte->tc, delta);

	spin_unlock_irqrestore(&ptp_dte->lock, flags);

	return 0;
}

static int ptp_dte_gettime(struct ptp_clock_info *ptp, struct timespec64 *ts)
{
	unsigned long flags;
	struct ptp_dte *ptp_dte = container_of(ptp, struct ptp_dte, caps);

	spin_lock_irqsave(&ptp_dte->lock, flags);
	*ts = ns_to_timespec64(timecounter_read(&ptp_dte->tc));
	spin_unlock_irqrestore(&ptp_dte->lock, flags);
	return 0;
}

static int ptp_dte_settime(struct ptp_clock_info *ptp,
			     const struct timespec64 *ts)
{
	unsigned long flags;
	struct ptp_dte *ptp_dte = container_of(ptp, struct ptp_dte, caps);
	s64 ns, new_ns;

	spin_lock_irqsave(&ptp_dte->lock, flags);

	new_ns = timespec64_to_ns(ts);
	ns = timecounter_read(&ptp_dte->tc);
	new_ns -= ns;
	timecounter_adjtime(&ptp_dte->tc, new_ns);

	spin_unlock_irqrestore(&ptp_dte->lock, flags);

	return 0;
}

static int ptp_dte_enable(struct ptp_clock_info *ptp,
			    struct ptp_clock_request *rq, int on)
{
	unsigned long flags;
	struct ptp_dte *ptp_dte = container_of(ptp, struct ptp_dte, caps);
	u32 val;
	u8 freq = (PTP_MAX_HZ / ptp_dte->freq) - 1 ;
	if ((PTP_MAX_HZ / ptp_dte->freq) > PTP_MAX_DIV_BIT) {
		dev_err(ptp_dte->dev, "%s: freq too slow should > %d\n", __func__, PTP_MAX_HZ / PTP_MAX_DIV_BIT);
		return -EINVAL;
	}
	
	spin_lock_irqsave(&nvttmr_lock, flags);
	if (on == PTP_TIMER_ENABLE) {

		val = readl(ptp_dte->regs + PTP_TIMER_IRQ_ENABLE);
		val |= PTP_MAX_VALUE;
		writel( val, (ptp_dte->regs + PTP_TIMER_IRQ_ENABLE));

		val = readl(ptp_dte->regs + PTP_TIMER_IRQ_BASE);
		val &= ~(PTP_TIMER_IRQ_VAL);
		writel( val, (ptp_dte->regs + PTP_TIMER_IRQ_BASE));

		val = readl(ptp_dte->regs + PTP_TIMER_DIV_BASE);
		val &= PTP_TIMER_DIV_DEF;
		val |= (freq << PTP_TIMER_FREQ_SHIFT) + PTP_TIMER_DIV_VAL;
		writel( val, (ptp_dte->regs + PTP_TIMER_DIV_BASE));

		val = readl(ptp_dte->regs + PTP_TIMER_TVAL_BASE);
		val |= PTP_MAX_VALUE;
		writel( val, (ptp_dte->regs + PTP_TIMER_TVAL_BASE));

		val = readl(ptp_dte->regs + PTP_TIMER_CNT_BASE);
		val &= ~(PTP_MAX_VALUE);
		writel( val, (ptp_dte->regs + PTP_TIMER_CNT_BASE));

		val = readl(ptp_dte->regs + PTP_TIMER_CTL_BASE);
		val |= PTP_TIMER_UP;
		writel( val, (ptp_dte->regs + PTP_TIMER_CTL_BASE));

	} else if (on == PTP_TIMER_DISABLE) { 

		val = readl(ptp_dte->regs + PTP_TIMER_CTL_BASE);
		val &= ~(PTP_TIMER_UP);
		writel( val, (ptp_dte->regs + PTP_TIMER_CTL_BASE));

	} else {
		dev_err(ptp_dte->dev, "%s: on/off should be set 1 or 0\n", __func__);
	}
	spin_unlock_irqrestore(&nvttmr_lock, flags);

	return 0;
}

static const struct ptp_clock_info ptp_dte_caps = {
	.owner		= THIS_MODULE,
	.name		= "DTE PTP timer",
	.max_adj	= 50000000,
	.n_ext_ts	= 0,
	.n_pins		= 0,
	.pps		= 0,
	.adjfreq	= ptp_dte_adjfreq,
	.adjtime	= ptp_dte_adjtime,
	.gettime64	= ptp_dte_gettime,
	.settime64	= ptp_dte_settime,
	.enable		= ptp_dte_enable,
};

static u64 ptp_cyclecounter_read(const struct cyclecounter *cc)
{
	return (u64)(dte_read_nco(ptp_dte->regs));
}

static int ptp_cyclecounter_init(struct ptp_dte *ptp)
{
	u64 start_count;
	u32 mult, shift;
	unsigned long flags;

	spin_lock_irqsave(&ptp_dte->lock, flags);

	
	start_count = (u64)ktime_to_ns(ktime_get_real());
	clocks_calc_mult_shift(&mult,&shift,ptp->freq,NSEC_PER_SEC,60);
	ptp->cc.read = ptp_cyclecounter_read;
	ptp->cc.mask = CLOCKSOURCE_MASK(32);
	ptp->cc.mult = mult;
	ptp->cc.shift = shift;

	timecounter_init(&ptp->tc,
			&ptp->cc, start_count);


	spin_unlock_irqrestore(&ptp_dte->lock, flags);
	return 0;
}

/*static irqreturn_t nvt_interrupt(int irq, void *dev_id)
{
	struct clock_event_device *clkevt = (struct clock_event_device *)dev_id;
	u32 val;
	unsigned long flags;
	int ret = IRQ_HANDLED;

	spin_lock_irqsave(&nvttmr_lock, flags);

	val = readl(ptp_dte->regs + PTP_TIMER_IRQ_STS);

	if (val & (1 << PTP_TIMER_NUM)) {
		val = (1 << PTP_TIMER_NUM);
		writel( val, (ptp_dte->regs + PTP_TIMER_IRQ_STS));
	}
	else
		ret = IRQ_NONE;

	spin_unlock_irqrestore(&nvttmr_lock, flags);

	return ret;
}*/

static int ptp_dte_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct resource *res;
	struct device_node *node = pdev->dev.of_node;
	u32 *pfreq;
	int ret = 0;

	if (MAX_TIMER_NUM < PTP_TIMER_NUM) {
		dev_err(dev, "Timer num not support\n");
		return -EINVAL;
	}
	if ((nvttmr_in_use & (1 << PTP_TIMER_NUM))) {
		dev_err(dev, "Timer num %d already in use\n",PTP_TIMER_NUM);
		return -EINVAL;
	} else {
		nvttmr_in_use |= (1 << PTP_TIMER_NUM);
	}
	ptp_dte = devm_kzalloc(dev, sizeof(struct ptp_dte), GFP_KERNEL);
	if (!ptp_dte)
		goto kzalloc_err;

#ifdef CONFIG_NOVATEK_TIMER
	ptp_dte->regs = nvttmr_base;
#else
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	ptp_dte->regs = devm_ioremap_resource(dev, res);
#endif
	if (IS_ERR(ptp_dte->regs)) {
		dev_err(dev, "%s: io remap failed\n", __func__);
		goto remap_err;
	}

#ifndef CONFIG_NOVATEK_TIMER
	spin_lock_init(&nvttmr_lock);
#endif
	spin_lock_init(&ptp_dte->lock);

	ptp_dte->clk = clk_get(dev, "f0040000.timer");
	if (IS_ERR(ptp_dte->clk)) {
		dev_err(dev, "%s:%d clk_get fail\n", __func__, __LINE__);
		goto clk_err;
	}
	if (clk_prepare_enable(ptp_dte->clk) != 0) {	
		dev_err(dev, "%s:%d enable clk fail\n", __func__, __LINE__);
		goto clk_err;
	}
	ptp_dte->freq = (u32)(clk_get_rate(ptp_dte->clk));
	
	pfreq = (u32 *)of_get_property(node, "clock-frequency", NULL);
	if (pfreq != NULL) {
		ptp_dte->freq = __be32_to_cpu(*pfreq);
	} 
	ptp_dte_enable(&ptp_dte->caps ,0,1);
	ptp_cyclecounter_init(ptp_dte);

	ptp_dte->dev = dev;
	ptp_dte->caps = ptp_dte_caps;

/*	int irq_num = platform_get_irq(pdev, 0);
	if (request_irq(irq_num, nvt_interrupt, IRQF_SHARED, "TIMER_TEST", ptp_dte->dev))
	{
		printk("%s:%d request_irq fail %d\n", __func__, __LINE__, irq_num);
		return -1;
	}*/
	return 0;
clk_err:
#ifndef CONFIG_NOVATEK_TIMER
	iounmap(ptp_dte->regs);
#endif
remap_err:
	kfree(ptp_dte);
kzalloc_err:
	nvttmr_in_use &= ~(1 << PTP_TIMER_NUM);

	return ret;
}

static int ptp_dte_remove(struct platform_device *pdev)
{
	nvttmr_in_use &= ~(1 << PTP_TIMER_NUM);

	if (ptp_dte->regs)
		ptp_dte_enable(&ptp_dte->caps , 0, PTP_TIMER_DISABLE);

	if (ptp_dte->clk) {
		clk_disable(ptp_dte->clk);
	}
	//free_irq(irq_num, ptp_dte->dev);	
#ifndef CONFIG_NOVATEK_TIMER
	if (ptp_dte->regs) {
		iounmap(ptp_dte->regs);
	}
#endif
	if (ptp_dte) {
		kfree(ptp_dte);
	}
	return 0;
}

static const struct of_device_id ptp_dte_of_match[] = {
	{ .compatible = "nvt,ptp-dte", },
	{},
};
MODULE_DEVICE_TABLE(of, ptp_dte_of_match);

static struct platform_driver ptp_dte_driver = {
	.driver = {
		.name = "ptp-dte",
		.pm = NULL,
		.of_match_table = ptp_dte_of_match,
	},
	.probe    = ptp_dte_probe,
	.remove   = ptp_dte_remove,
};
module_platform_driver(ptp_dte_driver);

MODULE_AUTHOR("Novatek");
MODULE_VERSION("1.0.0");
MODULE_DESCRIPTION("Novatek PTP Clock driver");
MODULE_LICENSE("GPL v2");
