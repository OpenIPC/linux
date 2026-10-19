/*
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/slab.h>
#include <linux/export.h>

#include <asm/io.h>
#include <asm/mach/time.h>

#include <mach/map.h>

#include <linux/delay.h>
#include "ak_timer_common.h"

//#define AK_HW_TIMER_BUG
#ifdef AK_HW_TIMER_BUG
#define AK_BUG pr_err
//#define AK_BUG  pr_info
#else
#define AK_BUG(fmt, arg...)
#endif


typedef int (*timer_handler)(void *data);

struct aktimer
{
#if defined(CONFIG_MACH_AK39EV330) || defined(CONFIG_MACH_AK37E) || \
    defined(CONFIG_MACH_AK3918AV100) || defined(CONFIG_MACH_AK3918EV300L)
        volatile unsigned int __force *ctrl1;
        volatile unsigned int __force *ctrl2;
#elif defined(CONFIG_MACH_AK3918AV130) || defined(CONFIG_MACH_KM01A) 
            volatile void __iomem *ctrl1;
#endif
    int timer_bit;
    int timer_num;
    int irq;
    timer_handler handler;
    void *data;
};

#ifdef CONFIG_MACH_AK37D
static int timers_irq_list[7] = {0};
static unsigned long flags;
#endif
#if defined(CONFIG_MACH_AK39EV330) || defined(CONFIG_MACH_AK37E) || \
    defined(CONFIG_MACH_AK3918AV100) || defined(CONFIG_MACH_AK3918EV300L)
static int timers_irq_list[10] = {0};
static unsigned long flags;
#elif defined(CONFIG_MACH_AK3918AV130) || defined(CONFIG_MACH_KM01A) 
static int timers_irq_list[5] = {0};
#endif

/* copy from plat-s3c/time.c
 *
 *  timer_mask_usec_ticks
 *
 * given a clock and divisor, make the value to pass into timer_ticks_to_usec
 * to scale the ticks into usecs
*/
static inline unsigned long
timer_mask_usec_ticks(unsigned long scaler, unsigned long pclk)
{
    unsigned long den = pclk / 1000;

    return ((1000 << TIMER_USEC_SHIFT) * scaler + (den >> 1)) / den;
}

#if defined(CONFIG_MACH_AK39EV330) || defined(CONFIG_MACH_AK37E) || \
    defined(CONFIG_MACH_AK3918AV100) || defined(CONFIG_MACH_AK3918EV300L)

/*
 * ak_timer_setup
 * @PARAM[in] ptimer
 */
static inline void ak_timer_setup(struct aktimer *ptimer)
{
    unsigned long regval;

    /* clear timeout puls, reload */
    regval = __raw_readl(ptimer->ctrl2);
    __raw_writel(regval | TIMER_CLEAR_BIT, ptimer->ctrl2);
}
#endif

/*
 * ak_timer_stop
 * @PARAM[in] priv
 */
int ak_timer_stop(void *priv)
{
    struct aktimer *ptimer = priv;

#if defined(CONFIG_MACH_AK3918AV130) || defined(CONFIG_MACH_KM01A) 
    //disable timer
    __raw_writel(0, ptimer->ctrl1 + CFG_TIMER_CTRL);
    //close interrupt
    __raw_writel(0, ptimer->ctrl1 + CFG_TIMER_INT_EN);
    //clear interrupt
    __raw_writel(1, ptimer->ctrl1 + CFG_TIMER_INTCLR);
#else
    __raw_writel(~TIMER_ENABLE_BIT, ptimer->ctrl2);
    clear_bit(ptimer->timer_bit, &flags);
#endif

    return 0;
}
EXPORT_SYMBOL(ak_timer_stop);

/*
 * IRQ handler for the timer
 * @PARAM[in] irq
 * @PARAM[in] dev_id
 */
static irqreturn_t ak_timer_interrupt(int irq, void *dev_id)
{
    struct aktimer *ptimer = dev_id;
#if defined(CONFIG_MACH_AK3918AV130) || defined(CONFIG_MACH_KM01A) 
    u32 ctrl2;
    ctrl2 = __raw_readl( ptimer->ctrl1 + RET_TIMER_INT_STA);
    if (ctrl2 & TIMER_INT_STA_BIT) 
    {
        ptimer->handler(ptimer->data);
        __raw_writel(AK_TIMER_INTCLR,  ptimer->ctrl1 + CFG_TIMER_INTCLR);
    }
#else
    if (__raw_readl(ptimer->ctrl2) & TIMER_STATUS_BIT)
    {

        ptimer->handler(ptimer->data);

        ak_timer_setup(ptimer);
    }
#endif

    return IRQ_HANDLED;
}

/*
 * ak_timer_probe
 * @PARAM[in]
 */
void *ak_timer_probe(int which_timer)
{
    int ret;
    int timer_bit;
    u32 timer_val;
    struct aktimer *ptimer;

    /* check if use by cs or ce */
    if ((which_timer == AK_CE_TIMER_INDEX) ||
        (which_timer == AK_CS_TIMER_INDEX))
    {
        pr_err("ak_hardware_timers can not use %d timers, "
               "it is reserved for clocksource and clockevent\n",
               which_timer);
        goto err1;
    }

    ptimer = kzalloc(sizeof(struct aktimer), GFP_KERNEL);
    if (ptimer == NULL)
    {
        pr_err("%s kmalloc failed.\n", __func__);
        goto err1;
    }

#if defined(CONFIG_MACH_AK3918AV130) || defined(CONFIG_MACH_KM01A) 
    switch (which_timer)
    {
    case 2:
        ptimer->ctrl1 = AK_TIMER2_CTRL;
        ptimer->irq = timers_irq_list[2];
        break;
    case 3:
        ptimer->ctrl1 = AK_TIMER3_CTRL;
        ptimer->irq = timers_irq_list[3];
        break;
    case 4:
        ptimer->ctrl1 = AK_TIMER4_CTRL;
        ptimer->irq = timers_irq_list[4];
        break;
#else
    switch (which_timer)
    {
    case 0:
        ptimer->ctrl1 = AK_TIMER0_CTRL1;
        ptimer->ctrl2 = AK_TIMER0_CTRL2;
        ptimer->irq = timers_irq_list[0];
        break;
    case 1:
        ptimer->ctrl1 = AK_TIMER1_CTRL1;
        ptimer->ctrl2 = AK_TIMER1_CTRL2;
        ptimer->irq = timers_irq_list[1];
        break;
    case 2:
        ptimer->ctrl1 = AK_TIMER2_CTRL1;
        ptimer->ctrl2 = AK_TIMER2_CTRL2;
        ptimer->irq = timers_irq_list[2];
        break;
    case 3:
        ptimer->ctrl1 = AK_TIMER3_CTRL1;
        ptimer->ctrl2 = AK_TIMER3_CTRL2;
        ptimer->irq = timers_irq_list[3];
        break;
    case 4:
        ptimer->ctrl1 = AK_TIMER4_CTRL1;
        ptimer->ctrl2 = AK_TIMER4_CTRL2;
        ptimer->irq = timers_irq_list[4];
        break;
    case 5:
        ptimer->ctrl1 = AK_TIMER5_CTRL1;
        ptimer->ctrl2 = AK_TIMER5_CTRL2;
        ptimer->irq = timers_irq_list[5];
        break;
#endif

#if defined(CONFIG_MACH_AK39EV330) || defined(CONFIG_MACH_AK37E) || \
    defined(CONFIG_MACH_AK3918AV100) || defined(CONFIG_MACH_AK3918EV300L)
    case 6:
        ptimer->ctrl1 = AK_TIMER6_CTRL1;
        ptimer->ctrl2 = AK_TIMER6_CTRL2;
        ptimer->irq = timers_irq_list[6];
        break;
    case 7:
        ptimer->ctrl1 = AK_TIMER7_CTRL1;
        ptimer->ctrl2 = AK_TIMER7_CTRL2;
        ptimer->irq = timers_irq_list[7];
        break;
    case 8:
        ptimer->ctrl1 = AK_TIMER8_CTRL1;
        ptimer->ctrl2 = AK_TIMER8_CTRL2;
        ptimer->irq = timers_irq_list[8];
        break;
    case 9:
        ptimer->ctrl1 = AK_TIMER8_CTRL1;
        ptimer->ctrl2 = AK_TIMER8_CTRL2;
        ptimer->irq = timers_irq_list[9];
        break;
#endif

    default:
#ifdef CONFIG_MACH_AK37D
        pr_err("ak_hardware_timers only support 7 normal timers(timer0~6).\n");
#endif
#if defined(CONFIG_MACH_AK39EV330) || defined(CONFIG_MACH_AK37E) || \
    defined(CONFIG_MACH_AK3918AV100) || defined(CONFIG_MACH_AK3918EV300L)
        pr_err("ak_hardware_timers only support 10 normal timers(timer0~9).\n");
#elif defined(CONFIG_MACH_AK3918AV130) || defined(CONFIG_MACH_KM01A) 
    pr_err("km01a support 5 normal timers(timer0~4). \n system enabled time0 and time1\n");
#endif
        goto err2;
        break;
    }

    ptimer->timer_num = which_timer;
    timer_bit = 1 << (which_timer - 1);
    ptimer->timer_bit = timer_bit;


#if defined(CONFIG_MACH_AK3918AV130) || defined(CONFIG_MACH_KM01A) 
        timer_val = __raw_readl(ptimer->ctrl1+CFG_TIMER_CTRL);
        if (timer_val & ENABLE_TIMER){
          pr_err("fail timer:%d is enabled don't start",
                ptimer->timer_num);
          pr_err("please reselect ak_timer_probe(int which_timer)\n");
          ret = -EINVAL;
           goto err1;
        }
#else
        timer_val = __raw_readl(ptimer->ctrl2);
        if (timer_val & TIMER_ENABLE_BIT){
          pr_err("fail timer:%d is enabled pwm %d don't start",
                ptimer->timer_num,ptimer->timer_num);
          pr_err("please reselect ak_timer_probe(int which_timer)\n");
          ret = -EINVAL;
           goto err1;
        }

#endif

    /* setup irq handler for IRQ_TIMER */
    ret = request_irq(ptimer->irq, ak_timer_interrupt,
                      0 /*IRQF_DISABLED*/ | IRQF_TIMER | IRQF_IRQPOLL,
                      "hwtimer", ptimer);
    if (ret)
    {
        pr_err("%s request irq for timer failed.\n", __func__);
        goto err2;
    }
    return ptimer;

err2:
    kfree(ptimer);
err1:
    return NULL;
}
/*end of func*/

EXPORT_SYMBOL(ak_timer_probe);

/*
 * ak_timer_remove
 * @PARAM[in]
 */
int ak_timer_remove(void *priv)
{
    struct aktimer *ptimer = priv;

    ak_timer_stop(priv);
    free_irq(ptimer->irq, ptimer);
    kfree(ptimer);

    return 0;
}
EXPORT_SYMBOL(ak_timer_remove);

/*
 * @BRIEF        ak_timer_start
 * @PARAM[in]    handler
 * @PARAM[in]    *data
 * @PARAM[in]    *priv
 * @PARAM[in]    hz
 * @RETURN       int
 * @RETVAL       ret
 */
int ak_timer_start(timer_handler handler, void *data, void *priv, int hz)
{
    int ret = 0;
    struct aktimer *ptimer = priv;
    unsigned long timecnt;
#if defined(CONFIG_MACH_AK3918AV130) || defined(CONFIG_MACH_KM01A)
    u32 timer_val;
#endif

    if (handler == NULL){
        pr_debug("%s handler NULL", __func__);
        ret = -EINVAL;
        goto err1;
    }

    ptimer->handler = handler;
    ptimer->data = data;


#if defined(CONFIG_MACH_AK3918AV130) || defined(CONFIG_MACH_KM01A) 
    timecnt = (12000000/hz) - 1;
    if(timecnt < 1)
        timecnt = 1;



    //1. 设置timer的TIMER_LOAD计数初值。
    __raw_writel(timecnt, ptimer->ctrl1 + CFG_TIMER_LOAD);
     AK_BUG("[ %s line:%d] CFG_TIMER_LOAD:%#08x\n",__func__,__LINE__, __raw_readl(ptimer->ctrl1+CFG_TIMER_LOAD));

     //2. 设置timer的CFG_TIMER_INT_EN数值，使能中断。
    __raw_writel(AK_TIMER_INT_EN, ptimer->ctrl1 + CFG_TIMER_INT_EN);
     AK_BUG("[ %s line:%d] CFG_TIMER_INT_EN:%#08x\n",__func__,__LINE__, __raw_readl(ptimer->ctrl1+CFG_TIMER_INT_EN));

     //3. 设置timer的CFG_TIMER_DIV分频因子。
    //写0表示256分频，写1表示1分频，以此类推，写255表示255分频，最大支持256分频
    timer_val = __raw_readl(ptimer->ctrl1+CFG_TIMER_DIV);
    timer_val &= ~(0xFF);
    timer_val |=  0x1;
    __raw_writel(timer_val, ptimer->ctrl1+CFG_TIMER_DIV);
    AK_BUG("[ %s line:%d] CFG_TIMER_DIV:%#08x\n",__func__,__LINE__, __raw_readl(ptimer->ctrl1+CFG_TIMER_DIV));


    /*4. 设置timer的CFG_TIMER_CTRL寄存器 工作模式*/
    timer_val = __raw_readl(ptimer->ctrl1+CFG_TIMER_CTRL);
    timer_val &= ~(AK_UPDATE_RELOAD);//don't update reload value
    timer_val &= ~(0x3 << 1);
    timer_val |= (0x1 << 1);
    __raw_writel(timer_val, ptimer->ctrl1+CFG_TIMER_CTRL);
    AK_BUG("[ %s line:%d] CFG_TIMER_CTRL:%#08x\n",__func__,__LINE__, __raw_readl(ptimer->ctrl1+CFG_TIMER_CTRL));

     /*5、第0位为1使能定时器*/
    timer_val = __raw_readl(ptimer->ctrl1+CFG_TIMER_CTRL);
    timer_val |=ENABLE_TIMER;
    __raw_writel(timer_val, ptimer->ctrl1+CFG_TIMER_CTRL);
    AK_BUG("[ %s line:%d] CFG_TIMER_CTRL:%#08x\n",__func__,__LINE__, __raw_readl(ptimer->ctrl1+CFG_TIMER_CTRL));

#else
    timecnt = (12000000 / hz) - 1;
    if(timecnt < 1)
        timecnt = 1;

    __raw_writel(timecnt, ptimer->ctrl1);
    __raw_writel((TIMER_ENABLE_BIT | TIMER_FEED_BIT | MODE_AUTO_RELOAD_TIMER),
                 ptimer->ctrl2);
#endif 

err1:
    return ret;
}
EXPORT_SYMBOL(ak_timer_start);


#if defined(CONFIG_MACH_AK3918AV130) || defined(CONFIG_MACH_KM01A)

/*
 * @BRIEF        ak_timer_mode_start
 * @PARAM[in]    handler
 * @PARAM[in]    *data
 * @PARAM[in]    *priv
 * @PARAM[mode]    timer工作模式
        0：自由计数模式（默认输出）:定时器持续计数，当计数值减到0时又自动回转到其最大值(0xFFFF_FFFF)，并继续计数。
        1：周期计数模式 :当计数值减到0时再次载入初值并继续计数。
        2：单次计数模式 :当定时器的计数值减到0时就停止计数并去除使能信号
 * @PARAM[in]    hz  0<hz<12MHZ
 * @PARAM[in]    time 秒级别的定时0<s<357
 * @RETURN       int
 * @RETVAL       ret
 */
int ak_timer_mode_start(timer_handler handler, void *data, void *priv,int mode, int hz,unsigned int second)
{
    int ret = 0;
    struct aktimer *ptimer = priv;
    unsigned long timecnt;
    u32 timer_val;

    if (handler == NULL){
        pr_debug("%s handler NULL", __func__);
        ret = -EINVAL;
        goto err1;
    }

    ptimer->handler = handler;
    ptimer->data = data;

    if(0<hz && hz<=12000000){
        timecnt = 12000000/hz;
    }else if(second > 0 && second<357){
        timecnt = second * 12000000;
    }else{
        pr_err("please set right s or hz about time: 0<hz<12MHZ or 0<s<357\n");
        return -1;
    }

    timecnt -=1;
    if(timecnt < 1)
        timecnt = 1;

    if(!(0<=mode && mode <=2)){
        pr_err("time mode type error,must mode:0/1/2\n");
        return -1;
    }

    if(__raw_readl(ptimer->ctrl1+CFG_TIMER_CTRL) & ENABLE_TIMER){
        pr_err("time is used\n");
    }


    //1. 设置timer的TIMER_LOAD计数初值。
    __raw_writel(timecnt, ptimer->ctrl1 + CFG_TIMER_LOAD);
     AK_BUG("[ %s line:%d] CFG_TIMER_LOAD:%#08x\n",__func__,__LINE__, __raw_readl(ptimer->ctrl1+CFG_TIMER_LOAD));

     //2. 设置timer的CFG_TIMER_INT_EN数值，使能中断。
    __raw_writel(AK_TIMER_INT_EN, ptimer->ctrl1 + CFG_TIMER_INT_EN);
     AK_BUG("[ %s line:%d] CFG_TIMER_INT_EN:%#08x\n",__func__,__LINE__, __raw_readl(ptimer->ctrl1+CFG_TIMER_INT_EN));

     //3. 设置timer的CFG_TIMER_DIV分频因子。
    //写0表示256分频，写1表示1分频，以此类推，写255表示255分频，最大支持256分频
    timer_val = __raw_readl(ptimer->ctrl1+CFG_TIMER_DIV);
    timer_val &= ~(0xFF);
    timer_val |=  0x1;
    __raw_writel(timer_val, ptimer->ctrl1+CFG_TIMER_DIV);
    AK_BUG("[ %s line:%d] CFG_TIMER_DIV:%#08x\n",__func__,__LINE__, __raw_readl(ptimer->ctrl1+CFG_TIMER_DIV));


    /*4. 设置timer的CFG_TIMER_CTRL寄存器 工作模式*/
    timer_val = __raw_readl(ptimer->ctrl1+CFG_TIMER_CTRL);
    timer_val &= ~(AK_UPDATE_RELOAD);//don't update reload value
    timer_val &= ~(0x3 << 1);
    timer_val |= mode << 1;
    __raw_writel(timer_val, ptimer->ctrl1+CFG_TIMER_CTRL);
    AK_BUG("[ %s line:%d] CFG_TIMER_CTRL:%#08x\n",__func__,__LINE__, __raw_readl(ptimer->ctrl1+CFG_TIMER_CTRL));

     /*5、第0位为1使能定时器*/
    timer_val = __raw_readl(ptimer->ctrl1+CFG_TIMER_CTRL);
    timer_val |=ENABLE_TIMER;
    __raw_writel(timer_val, ptimer->ctrl1+CFG_TIMER_CTRL);
    AK_BUG("[ %s line:%d] CFG_TIMER_CTRL:%#08x\n",__func__,__LINE__, __raw_readl(ptimer->ctrl1+CFG_TIMER_CTRL));



err1:
    return ret;
}
EXPORT_SYMBOL(ak_timer_mode_start);


#endif



/*
 * ak_timer_set_timers_irq - set irq of timers
 * @index:      index of irq, start from 0
 * @irq:        irq-number of the  irq
 */
int ak_timer_set_timers_irq(int index, int irq)
{
    timers_irq_list[index] = irq;
    return 0;
}
EXPORT_SYMBOL(ak_timer_set_timers_irq);
