/* drivers/rtc/rtc-ingenic_.c
 *
 * Real Time Clock interface for Ingenic's SOC, such as ingenic_x2000_v2.
 *
 *
 * Copyright (C) 2015 Ingenic Semiconductor Co., Ltd.
 *	http://www.ingenic.com
 * Author:	Wang Chengwan<cwwang@ingenic.cn>
 *		Aaron Wang<hfwang@ingenic.cn>
 *		Sun Jiwei <jiwei.sun@ingenic.com>
 *		wssong <wenshuo.song@ingenic.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/rtc.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/string.h>
#include <linux/clk.h>
#include <linux/bitops.h>
#include <linux/pm.h>

#include "rtc-ingenic.h"

/*FIXME by wssong,it should in board info*/
/* Default time for the first-time power on */
static struct rtc_time default_tm = {
	.tm_year = (2020 - 1900),	// year 2019
	.tm_mon = (3 - 1),		// month 3
	.tm_mday = 1,			// day 1
	.tm_hour = 12,
	.tm_min = 0,
	.tm_sec = 0
};

#ifndef RTC_DBUG_DUMP
unsigned long rtc_num=0;
unsigned long rtc_alarm_num=0;
#endif

static inline int rtc_periodic_alarm(struct rtc_time *tm)
{
	return  (tm->tm_year == -1) ||
		((unsigned)tm->tm_mon >= 12) ||
		((unsigned)(tm->tm_mday - 1) >= 31) ||
		((unsigned)tm->tm_hour > 23) ||
		((unsigned)tm->tm_min > 59) ||
		((unsigned)tm->tm_sec > 59);
}

static unsigned int ingenic_rtc_readl(struct ingenic_rtc *dev,int offset)
{
	unsigned int data, timeout = 0x100000;
	do {
		data = readl(dev->iomem + offset);
	} while (readl(dev->iomem + offset) != data && timeout--);
	if (timeout <= 0)
		pr_info("RTC : rtc_read_reg timeout!\n");
	return data;
}

static inline void wait_write_ready(struct ingenic_rtc *dev)
{
	int timeout = 0x100000;
	while (!(ingenic_rtc_readl(dev,RTC_RTCCR) & RTCCR_WRDY) && timeout--);
	if (timeout <= 0){
		pr_info("RTC : %s timeout!\n",__func__);
	}
}

static void ingenic_rtc_writel(struct ingenic_rtc *dev,int offset, unsigned int value)
{

	int timeout = 0x100000;
	mutex_lock(&dev->mutex_wr_lock);
	wait_write_ready(dev);
	writel(WENR_WENPAT_WRITABLE, dev->iomem + RTC_WENR);
	wait_write_ready(dev);
	while (!(ingenic_rtc_readl(dev,RTC_WENR) & WENR_WEN) && timeout--);
	if (timeout <= 0){
		pr_info("RTC :  wait_writable timeout!\n");
	}
	wait_write_ready(dev);
	writel(value,dev->iomem + offset);
	wait_write_ready(dev);
	mutex_unlock(&dev->mutex_wr_lock);
}

static inline void ingenic_rtc_clrl(struct ingenic_rtc *dev,int offset, unsigned int value)
{
	ingenic_rtc_writel(dev, offset, ingenic_rtc_readl(dev,offset) & ~(value));
}

static inline void ingenic_rtc_setl(struct ingenic_rtc *dev,int offset, unsigned int value)
{
	ingenic_rtc_writel(dev,offset,ingenic_rtc_readl(dev,offset) | (value));
}

#define IS_RTC_IRQ(x,y)  (((x) & (y)) == (y))

#ifndef RTC_DBUG_DUMP
static void ingenic_rtc_dump(struct ingenic_rtc *dev)
{
	printk ("******************************ingenic_rtc_dump**********************\n\n");
	printk ("ingenic_rtc_dump-----RTC_RTCCR is  --%#x--\n",ingenic_rtc_readl(dev, RTC_RTCCR));
	printk ("ingenic_rtc_dump-----RTC_RTCSR is  --%#x--\n",ingenic_rtc_readl(dev, RTC_RTCSR));
	printk ("ingenic_rtc_dump-----RTC_RTCSAR is --%#x--\n",ingenic_rtc_readl(dev,RTC_RTCSAR));
	printk ("ingenic_rtc_dump-----RTC_RTCGR is  --%#x--\n",ingenic_rtc_readl(dev, RTC_RTCGR));
	printk ("ingenic_rtc_dump-----RTC_HCR is    --%#x--\n",ingenic_rtc_readl(dev, RTC_HCR));
	printk ("ingenic_rtc_dump-----RTC_HWFCR is  --%#x--\n",ingenic_rtc_readl(dev, RTC_HWFCR));
	printk ("ingenic_rtc_dump-----RTC_HRCR is   --%#x--\n",ingenic_rtc_readl(dev, RTC_HRCR));
	printk ("ingenic_rtc_dump-----RTC_HWCR is   --%#x--\n",ingenic_rtc_readl(dev, RTC_HWCR));
	printk ("ingenic_rtc_dump-----RTC_HWRSR is  --%#x--\n",ingenic_rtc_readl(dev,RTC_HWRSR));
	printk ("ingenic_rtc_dump-----RTC_HSPR is   --%#x--\n",ingenic_rtc_readl(dev, RTC_HSPR));
	printk ("ingenic_rtc_dump-----RTC_WENR is   --%#x--\n",ingenic_rtc_readl(dev, RTC_WENR));
	printk ("ingenic_rtc_dump-----RTC_WKUPPINCR is --%#x--\n",ingenic_rtc_readl(dev,RTC_WKUPPINCR));
	printk ("ingenic_rtc_dump-----1HZIE :   --%#lx--\n",rtc_num);
	printk ("ingenic_rtc_dump-----ALARM :   --%#lx--\n",rtc_alarm_num);
	printk ("***************************ingenic_rtc_dump***************************\n");

	return;
}
#endif

static void ingenic_rtc_irq_work(struct work_struct *work)
{

	unsigned int rtsr,save_rtsr;
	unsigned long events;
	struct ingenic_rtc *rtc =  container_of(work, struct ingenic_rtc, work);

	rtsr = ingenic_rtc_readl(rtc, RTC_RTCCR);
	save_rtsr = rtsr;
	//is rtc interrupt
	events = 0;
	if (IS_RTC_IRQ(rtsr,RTCCR_AF)) {
        rtc_alarm_num++;
		events = RTC_AF | RTC_IRQF;
		rtsr &= ~RTCCR_AF;

		if (rtc_periodic_alarm(&rtc->rtc_alarm) == 0)
			rtsr &= ~(RTCCR_AIE | RTCCR_AE);
	}

	if (IS_RTC_IRQ(rtsr,RTCCR_1HZ)) {
        rtc_num++;
		rtsr &= ~(RTCCR_1HZ | RTCCR_1HZIE); //Turn off interrupt after triggering interrupt
		events = RTC_UF | RTC_IRQF;
	}

	if(events != 0)
		rtc_update_irq(rtc->rtc, 1, events);

	if(rtsr != save_rtsr)
		ingenic_rtc_writel(rtc, RTC_RTCCR,rtsr);

	enable_irq(rtc->irq);

	return;
}

static irqreturn_t ingenic_rtc_interrupt(int irq, void *dev_id)
{
	struct ingenic_rtc *rtc = (struct ingenic_rtc *) (dev_id);
	disable_irq_nosync(rtc->irq);
	schedule_work(&rtc->work);
	return IRQ_HANDLED;
}

static int ingenic_rtc_open(struct device *dev)
{
	return 0;
}

static void ingenic_rtc_release(struct device *dev)
{
     /*	struct ingenic_rtc *rtc = dev_get_drvdata(dev); */

     /*	free_irq(rtc->irq, rtc); */
}

/*add by wssong for rtc x2000 at 2019-07-06 begin*/
static void enter_hib_mode(struct ingenic_rtc *rtc)
{
	ingenic_rtc_setl(rtc,RTC_RTCCR,RTCCR_AE);
	ingenic_rtc_setl(rtc,RTC_HWCR,HWCR_EALM);
	ingenic_rtc_setl(rtc,RTC_HCR,HCR_PD);
}

static void wkuppincr_on(struct ingenic_rtc *rtc)
{
	ingenic_rtc_setl(rtc,RTC_WKUPPINCR,WKUPPINCR_BUFFEREN | WKUPPINCR_FASTBOOT
			| WKUPPINCR_RAMSLEEP | WKUPPINCR_RAMSHUT);
}

static void wkuppincr_off(struct ingenic_rtc *rtc)
{
	ingenic_rtc_clrl(rtc,RTC_WKUPPINCR,WKUPPINCR_BUFFEREN | WKUPPINCR_FASTBOOT
			| WKUPPINCR_RAMSLEEP | WKUPPINCR_RAMSHUT);
}

static void  set_1hzie_on(struct ingenic_rtc *rtc)
{
	ingenic_rtc_setl(rtc,RTC_RTCCR,RTCCR_1HZIE);
}

static void set_1hzie_off(struct ingenic_rtc *rtc)
{
	ingenic_rtc_clrl(rtc,RTC_RTCCR,RTCCR_1HZIE);
}

static void set_extend_press_reset_en(struct ingenic_rtc *rtc)
{
	ingenic_rtc_clrl(rtc,RTC_WKUPPINCR,WKUPPINCR_P_RST_CLEAR);
	ingenic_rtc_setl(rtc,RTC_WKUPPINCR,WKUPPINCR_P_RST_EN);
}

static void set_extend_press_reset_off(struct ingenic_rtc *rtc)
{
	ingenic_rtc_setl(rtc,RTC_WKUPPINCR,WKUPPINCR_P_RST_EN_DEF);
}

static int write_rtc_reg(struct ingenic_rtc *rtc,struct set_reg *prv)
{
    int timeout = 1000;
    do{
        ingenic_rtc_writel(rtc,prv->reg,prv->val);
    }while((prv->val != ingenic_rtc_readl(rtc,prv->reg)) && timeout--);

    if(timeout <= 0){
        printk("set rtc reg(%x) error\n",prv->reg);
        goto err;
    }

    return 0;
err:
    return -1;
}
static unsigned int read_rtc_reg(struct ingenic_rtc *rtc,struct set_reg *prv)
{
    return ingenic_rtc_readl(rtc, prv->reg);
}

static unsigned int ctrl_rtc_reg(struct device *dev,unsigned long arg)
{
    struct set_reg rv;
    int ret = -1;
	struct ingenic_rtc *rtc = dev_get_drvdata(dev);
    if (copy_from_user(&rv, (void *)arg, sizeof(struct set_reg))) {
        dev_err(dev, "set rtc wkup mode failed\n");
        ret = -EFAULT;
    }
    if(rv.ctrl == 0){//write
        ret = write_rtc_reg(rtc,&rv);
        if(ret != 0){
            dev_err(dev, "set rtc wkup failed\n");
            ret = -EFAULT;
        }
        dev_info(dev,"ioctl write:reg(%#hx),val(%#hx)\n",rv.reg,rv.val);
    }else if (rv.ctrl == 1){//read
        rv.val = read_rtc_reg(rtc,&rv);
        if (copy_to_user((void *)arg, &rv, sizeof(struct set_reg))) {
            dev_err(dev, "set rtc wkup mode failed\n");
            ret = -EFAULT;
        }
        ret = 0;
        dev_info(dev,"ioctl read:reg(%#hx),val(%#hx)\n",rv.reg,rv.val);
    }else{
        dev_err(dev, "set rtc reg failed\n");
    }

    return ret;
}


static int ingenic_rtc_ioctl(struct device *dev, unsigned int cmd,
		unsigned long arg)
{
    int ret = 0;
	struct ingenic_rtc *rtc = dev_get_drvdata(dev);
	mutex_lock(&rtc->ioctl_mutexlock);
	switch (cmd) {
		case CTRL_RTC_REG:
            ret = ctrl_rtc_reg(dev,arg);
			break;
		case SET_PD_STEP:
			enter_hib_mode(rtc);
			break;
		case SET_DDRBUFER_EN:
			wkuppincr_on(rtc);
			break;
		case SET_DDRBUFER_DISEN:
			wkuppincr_off(rtc);
			break;
		case SET_RTCCR_1HZIE_ON:
			set_1hzie_on(rtc);
			break;
		case SET_RTCCR_1HZIE_OFF:
			set_1hzie_off(rtc);
			break;
		case SET_WKUPPINCR_P_RST_EN:
			set_extend_press_reset_en(rtc);
			break;
		case SET_WKUPPINCR_P_RST_OFF:
			set_extend_press_reset_off(rtc);
			break;
		case PRINT_INFO:
			ingenic_rtc_dump(rtc);
			break;
		default:
			return 0;
	}
	mutex_unlock(&rtc->ioctl_mutexlock);
	return ret;
}

static int ingenic_rtc_set_time(struct device *dev, struct rtc_time *tm)
{
	struct ingenic_rtc *rtc = NULL;
	unsigned long time = 0;
	int ret = -1;

	if (dev) {
		rtc = dev_get_drvdata(dev);
	} else {
		return ret;
	}

	ret = rtc_tm_to_time(tm, &time);
	if (ret == 0)
		ingenic_rtc_writel(rtc, RTC_RTCSR, time);
	return ret;
}

static int ingenic_rtc_read_time(struct device *dev, struct rtc_time *tm)
{
	unsigned int tmp;
	struct ingenic_rtc *rtc = dev_get_drvdata(dev);

	tmp = ingenic_rtc_readl(rtc, RTC_RTCSR);
	rtc_time_to_tm(tmp, tm);

	if (rtc_valid_tm(tm) < 0) {
		/* Set the default time */
		ingenic_rtc_set_time(dev, &default_tm);
		tmp = ingenic_rtc_readl(rtc, RTC_RTCSR);
		rtc_time_to_tm(tmp, tm);
	}

	return 0;
}

static int ingenic_rtc_read_alarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	unsigned int rtc_rcr,tmp;
	struct ingenic_rtc *rtc = dev_get_drvdata(dev);

	tmp = ingenic_rtc_readl(rtc, RTC_RTCSAR);
	rtc_time_to_tm(tmp, &rtc->rtc_alarm);
	memcpy(&alrm->time, &rtc->rtc_alarm, sizeof(struct rtc_time));
	rtc_rcr = ingenic_rtc_readl(rtc, RTC_RTCCR);
	alrm->enabled = (rtc_rcr & RTCCR_AIE) ? 1 : 0;
	alrm->pending = (rtc_rcr & RTCCR_AF) ? 1 : 0;
	return 0;
}

static int ingenic_rtc_set_alarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	int ret = 0;
	unsigned long time;
	unsigned int tmp;
	struct ingenic_rtc *rtc = dev_get_drvdata(dev);

	mutex_lock(&rtc->mutexlock);
	if (alrm->enabled) {
		pr_debug("Next alarm year:%d, mon:%d, day:%d, "
				"hour:%d, min:%d, sec:%d\n",
				alrm->time.tm_year, alrm->time.tm_mon,
				alrm->time.tm_mday, alrm->time.tm_hour,
				alrm->time.tm_min, alrm->time.tm_sec);
		rtc_tm_to_time(&alrm->time,&time);
		ingenic_rtc_writel(rtc, RTC_RTCSAR, time);
		tmp = ingenic_rtc_readl(rtc, RTC_RTCCR);
		tmp &= ~RTCCR_AF;
		tmp |= RTCCR_AIE | RTCCR_AE;
		ingenic_rtc_writel(rtc, RTC_RTCCR, tmp);
	} else {
		ingenic_rtc_clrl(rtc,RTC_RTCCR, RTCCR_AIE | RTCCR_AE | RTCCR_AF);
	}
	mutex_unlock(&rtc->mutexlock);

	return ret;
}

static int ingenic_rtc_proc(struct device *dev, struct seq_file *seq)
{
	struct ingenic_rtc *rtc = dev_get_drvdata(dev);
	seq_printf(seq, "RTC regulator\t: 0x%08x\n",
			ingenic_rtc_readl(rtc, RTC_RTCGR));
	seq_printf(seq, "update_IRQ\t: %s\n",
			(ingenic_rtc_readl(rtc, RTC_RTCCR) & RTCCR_1HZIE) ? "yes" : "no");

	return 0;
}

static int ingenic_rtc_alarm_irq_enable(struct device *dev, unsigned int enabled)
{
	struct ingenic_rtc *rtc = dev_get_drvdata(dev);
	unsigned int tmp;

	mutex_lock(&rtc->mutexlock);
	if (enabled) {
		ingenic_rtc_clrl(rtc,RTC_RTCCR, RTCCR_AIE | RTCCR_AE | RTCCR_AF);
		tmp = ingenic_rtc_readl(rtc, RTC_RTCCR);
		tmp &= ~RTCCR_AF;
		tmp |= RTCCR_AIE | RTCCR_AE;
		ingenic_rtc_writel(rtc, RTC_RTCCR, tmp);
	} else {
		tmp = ingenic_rtc_readl(rtc, RTC_RTCCR);
		tmp &= ~(RTCCR_AF | RTCCR_AE | RTCCR_AIE);
		ingenic_rtc_writel(rtc, RTC_RTCCR, tmp);
	}
	mutex_unlock(&rtc->mutexlock);
	return 0;
}

static const struct rtc_class_ops ingenic_rtc_ops = {
	.open       = ingenic_rtc_open,
	.release    = ingenic_rtc_release,
	.ioctl      = ingenic_rtc_ioctl,
	.read_time  = ingenic_rtc_read_time,
	.set_time   = ingenic_rtc_set_time,
	.read_alarm = ingenic_rtc_read_alarm,
	.set_alarm  = ingenic_rtc_set_alarm,
	.proc       = ingenic_rtc_proc,
	.alarm_irq_enable	= ingenic_rtc_alarm_irq_enable,
};

static void ingenic_rtc_enable(struct ingenic_rtc *rtc)
{
	unsigned int cfc,hspr,rgr_1hz;
	unsigned long time = 0;

	/*
	 * When we are powered on for the first time, init the rtc and reset time.
	 *
	 * For other situations, we remain the rtc status unchanged.
	 */

//	cpm_set_clock(CGU_RTCCLK, 32768); //later to know if we need to set it ,it may decided by hardware.
	cfc = HSPR_RTCV;
	hspr = ingenic_rtc_readl(rtc, RTC_HSPR);
	rgr_1hz = ingenic_rtc_readl(rtc, RTC_RTCGR) & RTCGR_NC1HZ_MASK;

	if ((hspr != cfc) || (rgr_1hz != RTC_FREQ_DIVIDER)) {
		/* We are powered on for the first time !!! */

		pr_info("ingenic_rtc: rtc power on reset !\n");

		/* Set 32768 rtc clocks per seconds */
		ingenic_rtc_writel(rtc, RTC_RTCGR, RTC_FREQ_DIVIDER);

		/*FIXME,by jwsun, should not be in here*/
		/* Set minimum wakeup_n pin low-level assertion time for wakeup: 100ms */
		ingenic_rtc_writel(rtc, RTC_HWFCR, HWFCR_WAIT_TIME(100));
		ingenic_rtc_writel(rtc, RTC_HRCR, HRCR_WAIT_TIME(60));


		/* Reset to the default time */
		rtc_tm_to_time(&default_tm, &time);
		ingenic_rtc_writel(rtc, RTC_RTCSR, time);

		/* clear alarm register */
		ingenic_rtc_writel(rtc, RTC_RTCSAR, 0);

		/* start rtc */
		ingenic_rtc_writel(rtc, RTC_RTCCR, RTCCR_RTCE);
		ingenic_rtc_writel(rtc, RTC_HSPR, cfc);
	}

	/*FIXME,by jwsun, should not be in here*/
	/* clear all rtc flags */
	ingenic_rtc_writel(rtc, RTC_HWRSR, 0);

	/*FIXME,by wssong, should not be in here*/
	/* Set the default value of WKUPPINCR */
	if (rtc->priv->version_num == 2)
		ingenic_rtc_writel(rtc,RTC_WKUPPINCR, WKUPPINCR_DEFAULT_V1);
	else
		ingenic_rtc_writel(rtc,RTC_WKUPPINCR, WKUPPINCR_DEFAULT);

	/* enabled Power detect*/
	mutex_lock(&rtc->mutexlock);
	if (rtc->priv->version_num == 2)
		ingenic_rtc_writel(rtc, RTC_HWCR, (ingenic_rtc_readl(rtc, RTC_HWCR) & 0x1));
	else
		ingenic_rtc_writel(rtc, RTC_HWCR,((ingenic_rtc_readl(rtc, RTC_HWCR) & 0x1) | (EPDET_DEFAULT << 3)));

	mutex_unlock(&rtc->mutexlock);
}

static struct rtc_private priv_data1 = {
	.version_num = 1,
};

static struct rtc_private priv_data2 = {
	.version_num = 2,
};

static const struct of_device_id ingenic_rtc_of_match[] = {
	{ .compatible = "ingenic,rtc", .data = (struct priv_data *)&priv_data1, },
	{ .compatible = "ingenic,rtc-x1600", .data = (struct priv_data *)&priv_data2, },
	{},
};
MODULE_DEVICE_TABLE(of, ingenic_rtc_of_match);

static int ingenic_rtc_probe(struct platform_device *pdev)
{

	struct ingenic_rtc *rtc;
	struct clk *rtc_gate;
	const struct of_device_id *dev_match;
	int ret;

	rtc = kzalloc(sizeof(*rtc), GFP_KERNEL);
	if (!rtc) {
		ret = -ENOMEM;
		goto err_nomem;
	}

	dev_match =of_match_node(ingenic_rtc_of_match, pdev->dev.of_node);
	rtc->priv = (struct rtc_private *)dev_match->data;
	rtc_gate = devm_clk_get(&pdev->dev, "gate_rtc");
	if (IS_ERR(rtc_gate)) {
		dev_err(&pdev->dev, "cannot find RTC clock\n");
		ret = PTR_ERR(rtc_gate);
		goto err_mem;
	}

	clk_prepare_enable(rtc_gate);
	clk_put(rtc_gate);
	rtc->irq = platform_get_irq(pdev, 0);
	if (rtc->irq < 0) {
		dev_err(&pdev->dev, "no irq for rtc tick\n");
		ret = rtc->irq;
		goto err_nosrc;
	}

	rtc->res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (rtc->res == NULL) {
		dev_err(&pdev->dev, "failed to get memory region resource\n");
		ret = -ENXIO;
		goto err_nosrc;
	}

	rtc->res = request_mem_region(rtc->res->start,
			rtc->res->end - rtc->res->start+1, pdev->name);
	if (rtc->res == NULL) {
		dev_err(&pdev->dev, "failed to reserve memory region\n");
		ret = -EFAULT;
		goto err_mem;
	}

	rtc->iomem = ioremap_nocache(rtc->res->start,
			rtc->res->end - rtc->res->start + 1);
	if (rtc->iomem == NULL) {
		dev_err(&pdev->dev, "failed ioremap()\n");
		ret = -EFAULT;
		goto err_nomap;
	}

	platform_set_drvdata(pdev, rtc);
	device_init_wakeup(&pdev->dev, 1);

	rtc->rtc = rtc_device_register(pdev->name, &pdev->dev,
			&ingenic_rtc_ops, THIS_MODULE);
	if (IS_ERR(rtc->rtc)) {
		ret = PTR_ERR(rtc->rtc);
		dev_err(&pdev->dev, "Failed to register rtc device: %d\n", ret);
		goto err_unregister_rtc;
	}

	INIT_WORK(&rtc->work, ingenic_rtc_irq_work);
	mutex_init(&rtc->mutexlock);
	mutex_init(&rtc->ioctl_mutexlock);
	mutex_init(&rtc->mutex_wr_lock);

	ret = request_irq(rtc->irq, ingenic_rtc_interrupt,
			IRQF_TRIGGER_LOW | IRQF_SHARED,
			"rtc 1Hz and alarm", rtc);
	if (ret) {
		pr_debug("IRQ %d already in use.\n", rtc->irq);
		goto err_free_irq;
	}

	ingenic_rtc_enable(rtc);
	printk("ingenic RTC probe success \n");
	return 0;

err_free_irq:
	free_irq(rtc->irq,rtc);
err_unregister_rtc:
	rtc_device_unregister(rtc->rtc);
	iounmap(rtc->iomem);
err_nomap:
	release_mem_region(rtc->res->start, resource_size(rtc->res));
err_mem:
err_nosrc:
	kfree(rtc);
err_nomem:
	return ret;
}

static int ingenic_rtc_remove(struct platform_device *pdev)
{
	struct ingenic_rtc *rtc = platform_get_drvdata(pdev);

	ingenic_rtc_writel(rtc, RTC_RTCCR, 0);
	if (rtc->rtc)
		rtc_device_unregister(rtc->rtc);

	clk_disable(rtc->clk);
	clk_put(rtc->clk);
	rtc->clk = NULL;

	free_irq(rtc->irq,rtc);
	iounmap(rtc->iomem);
	release_resource(rtc->res);
	kfree(rtc);
	platform_set_drvdata(pdev, NULL);
	return 0;
}

#ifdef CONFIG_PM
static int ingenic_rtc_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct ingenic_rtc *rtc = platform_get_drvdata(pdev);

#ifdef CONFIG_SUSPEND_TEST
	unsigned int val;

	val = ingenic_rtc_readl(rtc, RTC_RTCCR);
	if(val & RTCCR_AE) {
		rtc->save_rtccr = val;
		rtc->os_alarm_time = ingenic_rtc_readl(rtc, RTC_RTCSAR);
	}
	val |= RTCCR_AIE | RTCCR_AE;
	ingenic_rtc_writel(rtc, RTC_RTCCR, val);

#if defined (CONFIG_ALARM_WAKEUP)
	unsigned int test_alarm_time, sr_time;
	sr_time = ingenic_rtc_readl(rtc, RTC_RTCSR);
	test_alarm_time = sr_time + CONFIG_SUSPEND_ALARM_TIME;
	if(rtc->os_alarm_time && rtc->os_alarm_time > sr_time \
	   && rtc->os_alarm_time < test_alarm_time)
		test_alarm_time =  rtc->os_alarm_time;
	ingenic_rtc_writel(rtc, RTC_RTCSAR, test_alarm_time);

    /* rtc disable power detect && RTC Alarm wakeup enable*/
	ingenic_rtc_writel(rtc,RTC_HWCR, (0x1a55a5a5 << 3) | 1);
	printk("-------alarm suspend count = %d\n", rtc->sleep_count++);
#elif defined (SUSPEND_KEY_WAKEUP)
	ingenic_rtc_writel(rtc,RTC_HWCR, (0x1a55a5a5 << 3));
	printk("-------key suspend count = %d\n", rtc->sleep_count++);
#else
    printk("No set rtc wakeup mode !!!");
#endif


#endif/*CONFIG_SUSPEND_TEST*/
	if (device_may_wakeup(&pdev->dev)) {
		enable_irq_wake(rtc->irq);
	}

	return 0;
}

static int ingenic_rtc_resume(struct platform_device *pdev)
{
	struct ingenic_rtc *rtc = platform_get_drvdata(pdev);

	if (device_may_wakeup(&pdev->dev)) {
		disable_irq_wake(rtc->irq);
	}
#ifdef CONFIG_SUSPEND_TEST
	if(rtc->save_rtccr & RTCCR_AE) {
		ingenic_rtc_writel(rtc, RTC_RTCSAR, rtc->os_alarm_time);
		ingenic_rtc_writel(rtc, RTC_RTCCR, rtc->save_rtccr);
		rtc->os_alarm_time = 0;
		rtc->save_rtccr = 0;
	} else {
		unsigned int val;
		val = ingenic_rtc_readl(rtc, RTC_RTCCR);
		val &= ~ (RTCCR_AF |RTCCR_AIE | RTCCR_AE);
		ingenic_rtc_writel(rtc, RTC_RTCCR, val);
	}

#endif
	return 0;
}
#else
#define ingenic_rtc_suspend	NULL
#define ingenic_rtc_resume	NULL
#endif
static struct platform_driver ingenic_rtc_driver = {
	.driver	= {
		.name = "rtc-ingenic",
		.of_match_table = ingenic_rtc_of_match,
	},
	.remove		= __exit_p(ingenic_rtc_remove),
	.suspend	= ingenic_rtc_suspend,
	.resume		= ingenic_rtc_resume,
};
module_platform_driver_probe(ingenic_rtc_driver, ingenic_rtc_probe);

MODULE_AUTHOR("Aaron <hfwang@ingenic.cn>");
MODULE_DESCRIPTION("Ingenic SoC Realtime Clock Driver (RTC)");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:ingenic-rtc");
