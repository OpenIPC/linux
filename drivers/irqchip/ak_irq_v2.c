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

#define AK_IRQ_MASK             (AK_VA_SYSCTRL  + 0x24)
#define AK_IRQ_STATUS           (AK_VA_SYSCTRL  + 0x4C)

#define AK_FIQ_MASK             (AK_VA_SYSCTRL  + 0x28)

#define AK_SYSCTRL_INT_MASK     (AK_VA_SYSCTRL  + 0x2C)
#define AK_SYSCTRL_INT_STATUS   (AK_VA_SYSCTRL  + 0x30)

#define AK_L2MEM_IRQ_ENABLE     (AK_VA_L2CTRL   + 0x9C)

#define NR_BANKS        2
#define SYSCTRL_IRQ     3

/* Fix bank_irq size To 32*/
static int bank_irqs[NR_BANKS] = {32, 32};

struct ak_irqchip_intc {
    struct irq_domain *domain;
    void __iomem *mask[NR_BANKS];
    void __iomem *status[NR_BANKS];
};

static void __iomem *reg_mask[] __initconst =
    { AK_IRQ_MASK, AK_SYSCTRL_INT_MASK };
static void __iomem *reg_status[] __initconst =
    {AK_IRQ_STATUS, AK_SYSCTRL_INT_STATUS};
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

    return;
}

static struct irq_chip ak_irq_chip = {
    .name = "module-irq",
    .irq_mask_ack = ak_mask_irq,
    .irq_mask = ak_mask_irq,
    .irq_unmask = ak_unmask_irq,
};

/**
 * @brief: system module irq handler
 * 
 * @author: caolianming
 * @param [in] irq: irq number
 * @param [in] *desc: irq info description
 */
static void ak_sysctrl_handler(struct irq_desc *desc)
{
    unsigned long regval_mask, regval_sta;
    unsigned long intpend;
    unsigned int inr;
    u32 virq;
    
    regval_mask = __raw_readl(intc.mask[1]);
    regval_sta =  __raw_readl(intc.status[1]);

    intpend = (regval_mask) & (regval_sta);

    while(intpend)
    {
        /*
         * __ffs() returns the bit position of the first bit set, where the
         * LSB is 0 and MSB is 31.  Zero input is undefined.
        */
        inr = __ffs(intpend);
    
        virq = irq_find_mapping(intc.domain, MAKE_HWIRQ(1, inr));
    
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
    int sysctrl_virq;

    /* 1st, clear all interrupts */
    __raw_readl(AK_IRQ_STATUS);
    __raw_readl(AK_SYSCTRL_INT_STATUS);

    /* 2nd, mask all interrutps */
    __raw_writel(0x0, AK_IRQ_MASK);
    __raw_writel(0x0, AK_FIQ_MASK);
    __raw_writel(0x0, AK_SYSCTRL_INT_MASK);

    /* mask all l2 interrupts */
    __raw_writel(0x0, AK_L2MEM_IRQ_ENABLE);

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
    sysctrl_virq = irq_find_mapping(intc.domain, MAKE_HWIRQ(0, SYSCTRL_IRQ));

    irq_set_chained_handler(sysctrl_virq, ak_sysctrl_handler);

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
