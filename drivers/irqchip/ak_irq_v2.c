/*
 * AKXX irq driver
 *
 * Copyright (C) 2021 Guangzhou Anyka Microelectronics Co., Ltd. 
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
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/io.h>
#include <linux/slab.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/irqchip.h>
#include <linux/irqdomain.h>
#include <asm/exception.h>
#include <asm/mach/irq.h>
#include <asm/irq.h>
#include <mach/map.h>
#include <mach/irqs.h>

/* Put the bank and irq (32 bits) into the hwirq */
#define MAKE_HWIRQ(b, n)    ((b << 5) | (n))
#define HWIRQ_BANK(i)       (i >> 5)
#define HWIRQ_BIT(i)        BIT(i & 0x1f)
#if defined(CONFIG_MACH_AK3918AV130) || defined(CONFIG_MACH_KM01A)
#define AK_IRQ_MASK             (AK_VA_INT_TIMER + 0x04)
#define AK_IRQ_STATUS           (AK_VA_INT_TIMER + 0x08)

#define AK_FIQ_MASK             (AK_VA_INT_TIMER + 0x00)

#define AK_SYSCTRL_INT_MASK     (AK_VA_SYSCTRL  + 0x2C)
#define AK_SYSCTRL_INT_STATUS   (AK_VA_SYSCTRL  + 0x30)

#if defined(CONFIG_MACH_AK3918AV130)
/*******************************irq of SSS*******************************/
#define AK_SSS_INT_MASK         (AK_VA_INT_TIMER + 0x14)
#define AK_SSS_INT_STATUS       (AK_VA_INT_TIMER + 0x18)
/*******************************END**************************************/

/*******************************irq of NSS*******************************/
#define AK_NSS_INT_MASK         (AK_VA_INT_TIMER + 0x1C)
#define AK_NSS_INT_STATUS       (AK_VA_INT_TIMER + 0x20)
/*******************************END**************************************/

/*******************************irq of ISS*******************************/
#define AK_ISS_INT_MASK         (AK_VA_INT_TIMER + 0x24)
#define AK_ISS_INT_STATUS       (AK_VA_INT_TIMER + 0x28)
/*******************************END**************************************/

/*******************************irq of ESS*******************************/
#define AK_ESS_INT_MASK         (AK_VA_INT_TIMER + 0x2C)
#define AK_ESS_INT_STATUS       (AK_VA_INT_TIMER + 0x30)
/*******************************END**************************************/

/*******************************irq of ESS*******************************/
#define AK_TWI_INT_MASK         (AK_VA_INT_TIMER + 0x0C)
#define AK_TWI_INT_STATUS       (AK_VA_INT_TIMER + 0x10)
/*******************************END**************************************/

#endif

#define AK_L2MEM_IRQ_ENABLE     (AK_VA_L2CTRL   + 0x9C)
#else
#define AK_IRQ_MASK             (AK_VA_SYSCTRL  + 0x24)
#define AK_IRQ_STATUS           (AK_VA_SYSCTRL  + 0x4C)

#define AK_FIQ_MASK             (AK_VA_SYSCTRL  + 0x28)

#define AK_SYSCTRL_INT_MASK     (AK_VA_SYSCTRL  + 0x2C)
#define AK_SYSCTRL_INT_STATUS   (AK_VA_SYSCTRL  + 0x30)

#define AK_L2MEM_IRQ_ENABLE     (AK_VA_L2CTRL   + 0x9C)
#endif

#if defined(CONFIG_MACH_KM01A)
#define NR_BANKS        2
#define SYSCTRL_IRQ     3/*bit3:system ctrl*/
#elif defined(CONFIG_MACH_AK3918AV130)
#define NR_BANKS        7/*bank:TOP->system->SSS->ESS->ISS->NSS*/
#define SSS_IRQ         17/*bit17:SSS*/
#define SYSCTRL_IRQ     3/*bit3:system ctrl*/
#define ESS_IRQ         2/*bit2:ESS*/
#define ISS_IRQ         1/*bit1:ISS*/
#define NSS_IRQ         0/*bit0:NSS*/
#define TWI_IRQ         13/*bit0:NSS*/
#endif

/* Fix bank_irq size To 32*/
#if defined(CONFIG_MACH_KM01A)
static int bank_irqs[NR_BANKS] = {32, 32};
#elif defined(CONFIG_MACH_AK3918AV130)
/*
 * BANK0: TOP
 * BANK1: SYSTEM CONTROLL
 * BANK2: SSS
 * BANK3: ESS
 * BANK4: ISS
 * BANK5: NSS
 * BANK6: TWI
 */
static int bank_irqs[NR_BANKS] = {32, 32, 32, 32 ,32, 32, 32};
#endif

struct ak_irqchip_intc {
    struct irq_domain *domain;
    void __iomem *mask[NR_BANKS];
    void __iomem *status[NR_BANKS];
};

#if defined(CONFIG_MACH_KM01A)
static void __iomem *reg_mask[] __initconst =
    { AK_IRQ_MASK, AK_SYSCTRL_INT_MASK };
static void __iomem *reg_status[] __initconst =
    {AK_IRQ_STATUS, AK_SYSCTRL_INT_STATUS};
#elif defined(CONFIG_MACH_AK3918AV130)
static void __iomem *reg_mask[] __initconst =
    { AK_IRQ_MASK, AK_SYSCTRL_INT_MASK, AK_SSS_INT_MASK, AK_ESS_INT_MASK, AK_ISS_INT_MASK, AK_NSS_INT_MASK,\
      AK_TWI_INT_MASK};
static void __iomem *reg_status[] __initconst =
    {AK_IRQ_STATUS, AK_SYSCTRL_INT_STATUS, AK_SSS_INT_STATUS, AK_ESS_INT_STATUS, AK_ISS_INT_STATUS, AK_NSS_INT_STATUS,\
     AK_TWI_INT_STATUS};
#endif
static struct ak_irqchip_intc intc __read_mostly;

/*
 * Disable interrupt number "irq"
 */
static void ak_mask_irq(struct irq_data *d)
{
    unsigned long regval;

    pr_debug("%s irq=%u, hwirq=%lu\n", __func__, d->irq, d->hwirq);

    regval = __raw_readl(intc.mask[HWIRQ_BANK(d->hwirq)]);
    regval &= ~(HWIRQ_BIT(d->hwirq));       
    __raw_writel(regval, intc.mask[HWIRQ_BANK(d->hwirq)]);

    regval = __raw_readl(AK_VA_SYSCTRL); // flush 0x08000000
    return;
}

/*
 * Enable interrupt number "irq"
 */
static void ak_unmask_irq(struct irq_data *d)
{
    unsigned long regval;

    pr_debug("%s irq=%u, hwirq=%lu\n", __func__, d->irq, d->hwirq);

    regval = __raw_readl(intc.mask[HWIRQ_BANK(d->hwirq)]);
    regval |= HWIRQ_BIT(d->hwirq);  
    __raw_writel(regval, intc.mask[HWIRQ_BANK(d->hwirq)]);

    regval = __raw_readl(AK_VA_SYSCTRL); // flush 0x08000000
    return;
}

static struct irq_chip ak_irq_chip = {
    .name = "module-irq",
    .irq_mask_ack = ak_mask_irq,
    .irq_mask = ak_mask_irq,
    .irq_unmask = ak_unmask_irq,
};

static void ak_level2_handler(struct irq_desc *desc, int bit)
{
    unsigned long regval_mask, regval_sta;
    unsigned long intpend;
    unsigned int inr;
    u32 virq;
    
    regval_mask = __raw_readl(intc.mask[bit]);
    regval_sta =  __raw_readl(intc.status[bit]);

    intpend = (regval_mask) & (regval_sta);

    while(intpend)
    {
        /*
         * __ffs() returns the bit position of the first bit set, where the
         * LSB is 0 and MSB is 31.  Zero input is undefined.
        */
        inr = __ffs(intpend);
    
        virq = irq_find_mapping(intc.domain, MAKE_HWIRQ(bit, inr));
    
        /* 
         *  generic_handle_irq:
         *  Invoke the handler for a particular virq
         */
        generic_handle_irq(virq);
        intpend &= ~(BIT(inr));
        //pr_info("->* %x\n", __raw_readl(intc.status[1]));
    }
    return;
}

static void ak_sysctrl_handler(struct irq_desc *desc)
{
    ak_level2_handler(desc, 1);
}

#if defined(CONFIG_MACH_AK3918AV130)
static void ak_sss_handler(struct irq_desc *desc)
{
    ak_level2_handler(desc, 2);
}

static void ak_ess_handler(struct irq_desc *desc)
{
    ak_level2_handler(desc, 3);
}

static void ak_iss_handler(struct irq_desc *desc)
{
    ak_level2_handler(desc, 4);
}

static void ak_nss_handler(struct irq_desc *desc)
{
    ak_level2_handler(desc, 5);
}

static void ak_twi_handler(struct irq_desc *desc)
{
    ak_level2_handler(desc, 6);
}
#endif

static void __exception_irq_entry anyka_handle_irq(
    struct pt_regs *regs)
{
    unsigned long regval_mask, regval_sta;
    unsigned long intpend;

    regval_mask = __raw_readl(intc.mask[0]);
    regval_sta =  __raw_readl(intc.status[0]);
    intpend = (regval_mask) & (regval_sta);

    /*
    * __ffs() returns the bit position of the first bit set, where the
    * LSB is 0 and MSB is 31.  Zero input is undefined.
    */
    if(intpend)
        handle_domain_irq(intc.domain, MAKE_HWIRQ(0, __ffs(intpend)), regs);
    //pr_info("* %x\n", __raw_readl(intc.status[0]));
    return;
}


static int int_xlate(struct irq_domain *d, struct device_node *ctrlr,
    const u32 *intspec, unsigned int intsize,
    unsigned long *out_hwirq, unsigned int *out_type)
{
    if (WARN_ON(intsize != 2))
        return -EINVAL;

    if (WARN_ON(intspec[1] >= NR_BANKS))
        return -EINVAL;

    if (WARN_ON(intspec[0] > bank_irqs[intspec[1]]))  
        return -EINVAL;

    *out_hwirq = MAKE_HWIRQ(intspec[1], intspec[0]);
    *out_type = IRQ_TYPE_NONE;
    
    return 0;
}

static const struct irq_domain_ops intc_ops = {
    .xlate = int_xlate
};


static int __init ak_int_of_init(struct device_node *node,
                      struct device_node *parent)
{
    int b, i;
    int virq;
#if defined(CONFIG_MACH_KM01A)
    int sysctrl_virq;
#elif defined(CONFIG_MACH_AK3918AV130)
    int sysctrl_virq, sss_virq, ess_virq, iss_virq, nss_virq, twi_virq;
#endif

    /* 1st, clear all interrupts */
    __raw_readl(AK_IRQ_STATUS);
    __raw_readl(AK_SYSCTRL_INT_STATUS);
#if defined(CONFIG_MACH_AK3918AV130)
    __raw_readl(AK_SSS_INT_STATUS);
    __raw_readl(AK_ESS_INT_STATUS);
    __raw_readl(AK_ISS_INT_STATUS);
    __raw_readl(AK_NSS_INT_STATUS);
    __raw_readl(AK_TWI_INT_STATUS);
#endif

    /* 2nd, mask all interrutps */
    __raw_writel(0x0, AK_IRQ_MASK);
    __raw_writel(0x0, AK_FIQ_MASK);
    __raw_writel(0x0, AK_SYSCTRL_INT_MASK);
#if defined(CONFIG_MACH_AK3918AV130)
    __raw_writel(0x0, AK_SSS_INT_MASK);
    __raw_writel(0x0, AK_ESS_INT_MASK);
    __raw_writel(0x0, AK_ISS_INT_MASK);
    __raw_writel(0x0, AK_NSS_INT_MASK);
    __raw_writel(0x0, AK_TWI_INT_MASK);
#endif

    /* mask all l2 interrupts */
    __raw_writel(0x0, AK_L2MEM_IRQ_ENABLE);

    __raw_readl(AK_VA_SYSCTRL); // flush 0x08000000

    intc.domain = irq_domain_add_linear(node, MAKE_HWIRQ(NR_BANKS, 0),
            &intc_ops, NULL);

    if (!intc.domain)
        pr_err("%s: unable to create IRQ domain\n", node->full_name);

    for (b = 0; b < NR_BANKS; b++) {

        intc.mask[b]  = reg_mask[b];
        intc.status[b]  = reg_status[b];

        for (i = 0; i < bank_irqs[b]; i++) {
            virq = irq_create_mapping(intc.domain, MAKE_HWIRQ(b, i));
            pr_debug("b = %d, i = %d, hwirq =%d, virq = %d\n", b,  i,
                MAKE_HWIRQ(b, i), virq);
            BUG_ON(virq <= 0);
            irq_set_chip_and_handler(virq, &ak_irq_chip, handle_level_irq);
            irq_set_probe(virq);
        }
    }

    /*system ctrl*/
    sysctrl_virq = irq_find_mapping(intc.domain, MAKE_HWIRQ(0, SYSCTRL_IRQ));

    irq_set_chained_handler(sysctrl_virq, ak_sysctrl_handler);

#if defined(CONFIG_MACH_AK3918AV130)
    /*SSS*/
    sss_virq = irq_find_mapping(intc.domain, MAKE_HWIRQ(0, SSS_IRQ));

    irq_set_chained_handler(sss_virq, ak_sss_handler);

    /*ESS*/
    ess_virq = irq_find_mapping(intc.domain, MAKE_HWIRQ(0, ESS_IRQ));

    irq_set_chained_handler(ess_virq, ak_ess_handler);

    /*ISS*/
    iss_virq = irq_find_mapping(intc.domain, MAKE_HWIRQ(0, ISS_IRQ));

    irq_set_chained_handler(iss_virq, ak_iss_handler);

    /*NSS*/
    nss_virq = irq_find_mapping(intc.domain, MAKE_HWIRQ(0, NSS_IRQ));

    irq_set_chained_handler(nss_virq, ak_nss_handler);

    /*TWI*/
    twi_virq = irq_find_mapping(intc.domain, MAKE_HWIRQ(0, TWI_IRQ));

    irq_set_chained_handler(twi_virq, ak_twi_handler);
#endif

#ifdef CONFIG_MULTI_IRQ_HANDLER
    set_handle_irq(anyka_handle_irq);
#endif

    return 0;
}


#if defined(CONFIG_MACH_AK3918AV100)
IRQCHIP_DECLARE(ak3918av1xx_irqchip, "anyka,ak3918av100-ic",
                ak_int_of_init);
#endif

#if defined(CONFIG_MACH_AK3918EV300L)
IRQCHIP_DECLARE(ak3918ev300l_irqchip, "anyka,ak3918ev300l-ic",
                ak_int_of_init);
#endif
#if defined(CONFIG_MACH_AK3918AV130)
IRQCHIP_DECLARE(ak3918av130_irqchip, "anyka,ak3918av130-ic",
                ak_int_of_init);
#endif
#if defined(CONFIG_MACH_KM01A)
IRQCHIP_DECLARE(km01a_irqchip, "anyka,km01a-ic",
                ak_int_of_init);
#endif
