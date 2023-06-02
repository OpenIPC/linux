/*
 * arch/arm/mach-gk/irq.c
 *
 * Author: Steven Yu, <yulindeng@gokemicro.com>
 * Copyright (C) 2012-2015, goke, Inc.
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
#include <linux/irqdomain.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/linkage.h>

#include <linux/module.h>

#include <mach/io.h>
#include <mach/gpio.h>
#include <mach/irqs.h>
#include <mach/uart.h>

#include <asm/exception.h>

#ifdef CONFIG_ARM_VIC
#include <asm/hardware/vic.h>
#endif

/**
 * struct vic_device - VIC PM device
 * @irq: The IRQ number for the base of the VIC.
 * @base: The register base for the VIC.
 * @resume_sources: A bitmask of interrupts for resume.
 * @resume_irqs: The IRQs enabled for resume.
 * @int_select: Save for VIC_INT_SELECT.
 * @int_enable: Save for VIC_INT_ENABLE.
 * @soft_int: Save for VIC_INT_SOFT.
 * @protect: Save for VIC_PROTECT.
 * @domain: The IRQ domain for the VIC.
 */
struct gk_vic_device {
    void __iomem    *base;
    int     irq;
    u32     resume_sources;
    u32     resume_irqs;
    u32     int_select;
    u32     int_enable;
    u32     soft_int;
    u32     protect;
    struct irq_domain *domain;
};

/* ==========================================================================*/

/* we cannot allocate memory when VICs are initially registered */
static struct gk_vic_device gk_vic_decs[VIC_INSTANCES];

static int gk_vic_id;
/* ==========================================================================*/

/* ==========================================================================*/
#define GK_GPIO_IRQ2GIRQ()   do { \
    girq -= GPIO_IRQ(0); \
    } while (0)


/* ==========================================================================*/
#if (VIC_INSTANCES == 1)
#define GK_VIC_IRQ2BASE()    do { \
    irq -= VIC1_INT_VEC(0); \
    } while (0)
#elif (VIC_INSTANCES == 2)
#define GK_VIC_IRQ2BASE()    do { \
    irq -= VIC1_INT_VEC(0); \
    if (irq >= NR_VIC_IRQ_SIZE) { \
        irq -= NR_VIC_IRQ_SIZE; \
        vic_base = VIC2_BASE; \
    } \
    } while (0)
#endif

static void gk_ack_irq(struct irq_data *d)
{
    u32                 vic_base = VIC1_BASE;
    u32                 irq = d->irq;

    GK_VIC_IRQ2BASE();

    gk_irq_writel(vic_base + VIC_EDGE_CLR_OFFSET, 0x1 << irq);
}

static void gk_disable_irq(struct irq_data *d)
{
    u32                 vic_base = VIC1_BASE;
    u32                 irq = d->irq;

    GK_VIC_IRQ2BASE();

    gk_irq_writel(vic_base + VIC_INTEN_CLR_OFFSET, 0x1 << irq);
}

static void gk_enable_irq(struct irq_data *d)
{
    u32                 vic_base = VIC1_BASE;
    u32                 irq = d->irq;

    GK_VIC_IRQ2BASE();

    gk_irq_writel(vic_base + VIC_INTEN_OFFSET, 0x1 << irq);
}

static void gk_mask_ack_irq(struct irq_data *d)
{
    u32                 vic_base = VIC1_BASE;
    u32                 irq = d->irq;

    GK_VIC_IRQ2BASE();

    gk_irq_writel(vic_base + VIC_INTEN_CLR_OFFSET, 0x1 << irq);
    gk_irq_writel(vic_base + VIC_EDGE_CLR_OFFSET, 0x1 << irq);
}

static int gk_irq_set_type(struct irq_data *d, unsigned int type)
{
    u32                 vic_base = VIC1_BASE;
    u32                 mask;
    u32                 bit;
    u32                 sense;
    u32                 bothedges;
    u32                 event;
    struct irq_desc             *desc = irq_to_desc(d->irq);
    u32                 irq = d->irq;

    //printk("%s: irq[%d] type[%d] desc[%p]\n", __func__, irq, type, desc);

    GK_VIC_IRQ2BASE();

    mask = ~(0x1 << irq);
    bit = (0x1 << irq);
    sense = gk_irq_readl(vic_base + VIC_SENSE_OFFSET);
    bothedges = gk_irq_readl(vic_base + VIC_BOTHEDGE_OFFSET);
    event = gk_irq_readl(vic_base + VIC_EVENT_OFFSET);

    switch (type) {
    case IRQ_TYPE_EDGE_RISING:
        sense &= mask;
        bothedges &= mask;
        event |= bit;
        desc->handle_irq = handle_edge_irq;
        break;
    case IRQ_TYPE_EDGE_FALLING:
        sense &= mask;
        bothedges &= mask;
        event &= mask;
        desc->handle_irq = handle_edge_irq;
        break;
    case IRQ_TYPE_EDGE_BOTH:
        sense &= mask;
        bothedges |= bit;
        event &= mask;
        desc->handle_irq = handle_edge_irq;
        break;
    case IRQ_TYPE_LEVEL_HIGH:
        sense |= bit;
        bothedges &= mask;
        event |= bit;
        desc->handle_irq = handle_level_irq;
        break;
    case IRQ_TYPE_LEVEL_LOW:
        sense |= bit;
        bothedges &= mask;
        event &= mask;
        desc->handle_irq = handle_level_irq;
        break;
    default:
        pr_err("%s: can't set irq type %d for irq 0x%08x@%d\n",
            __func__, type, vic_base, irq);
        return -EINVAL;
    }

    gk_irq_writel(vic_base + VIC_SENSE_OFFSET, sense);
    gk_irq_writel(vic_base + VIC_BOTHEDGE_OFFSET, bothedges);
    gk_irq_writel(vic_base + VIC_EVENT_OFFSET, event);

    gk_ack_irq(d);

    return 0;
}

static struct irq_chip gk_irq_chip = {
    .name       = "gk irq",
    .irq_ack    = gk_ack_irq,
    .irq_mask   = gk_disable_irq,
    .irq_unmask = gk_enable_irq,
    .irq_mask_ack   = gk_mask_ack_irq,
    .irq_set_type   = gk_irq_set_type,
};

void gk_swvic_set(u32 irq)
{
    u32                 vic_base = VIC1_BASE;

    GK_VIC_IRQ2BASE();

    gk_irq_writel(vic_base + VIC_SOFTEN_OFFSET, 0x1 << irq);
}
EXPORT_SYMBOL(gk_swvic_set);

void gk_swvic_clr(u32 irq)
{
    u32                 vic_base = VIC1_BASE;

    GK_VIC_IRQ2BASE();

    gk_irq_writel(vic_base + VIC_SOFTEN_CLR_OFFSET, 0x1 << irq);
}
EXPORT_SYMBOL(gk_swvic_clr);

/* ==========================================================================*/
static inline u32 gk_irq_stat2nr(u32 stat)
{
    u32                 tmp;
    u32                 nr;

    __asm__ __volatile__
        ("rsbs  %[tmp], %[stat], #0" :
        [tmp] "=r" (tmp) : [stat] "r" (stat));
    __asm__ __volatile__
        ("and   %[nr], %[tmp], %[stat]" :
        [nr] "=r" (nr) : [tmp] "r" (tmp), [stat] "r" (stat));
    __asm__ __volatile__
        ("clzcc %[nr], %[nr]" :
        [nr] "+r" (nr));
    __asm__ __volatile__
        ("rsc   %[nr], %[nr], #32" :
        [nr] "+r" (nr));

    return nr;
}

/* ==========================================================================*/

u32 vic_base_addr[VIC_INSTANCES] = {
    VIC1_BASE,
#if (VIC_INSTANCES >= 2)
    VIC2_BASE,
#endif
#if (VIC_INSTANCES >= 3)
    VIC3_BASE,
#endif
#if (VIC_INSTANCES >= 4)
    VIC4_BASE,
#endif
};

#ifndef CONFIG_ARM_VIC

/**
 * gk_vic_register() - Register a VIC.
 * @base: The base address of the VIC.
 * @irq: The base IRQ for the VIC.
 * @resume_sources: bitmask of interrupts allowed for resume sources.
 * @node: The device tree node associated with the VIC.
 *
 * Register the VIC with the system device tree so that it can be notified
 * of suspend and resume requests and ensure that the correct actions are
 * taken to re-instate the settings on resume.
 *
 * This also configures the IRQ domain for the VIC.
 */
static void __init gk_vic_register(void __iomem *base, unsigned int irq,
                u32 resume_sources, struct device_node *node)
{
    struct gk_vic_device *pVic;

    if (gk_vic_id >= ARRAY_SIZE(gk_vic_decs))
    {
        printk(KERN_ERR "%s: too few VICs, increase CONFIG_ARM_VIC_NR\n", __func__);
        return;
    }

    pVic = &gk_vic_decs[gk_vic_id];
    pVic->base = base;
    pVic->resume_sources = resume_sources;
    pVic->irq = irq;
    gk_vic_id++;
    pVic->domain = irq_domain_add_legacy(node, NR_VIC_IRQ_SIZE, irq, 0,
                      &irq_domain_simple_ops, pVic);
}

void __init gk_init_vic(u32 num_vic)
{
    int irq, i;

    printk("gk init vic...\n");
    for (i = 0; i < NR_IRQS; i++)
    {
        //pr_err("registering irq %d \n", i);
        irq_set_chip_and_handler(i, &gk_irq_chip, handle_level_irq);
        set_irq_flags(i, IRQF_VALID);
    }

    /* initialize the VICs */
    for (irq = 0; irq < num_vic; irq++)
    {
        gk_vic_register((void __iomem *)vic_base_addr[irq], (NR_VIC_IRQ_SIZE * irq), 0, NULL);
    }
}

/*
 * Handle each interrupt in a single VIC.  Returns non-zero if we've
 * handled at least one interrupt.  This reads the status register
 * before handling each interrupt, which is necessary given that
 * handle_IRQ may briefly re-enable interrupts for soft IRQ handling.
 */
static int gk_handle_one_vic(struct gk_vic_device *vic, struct pt_regs *regs)
{
    u32 stat, irq;
    int handled = 0;

    while ((stat = readl_relaxed(vic->base + VIC_IRQ_STA_OFFSET))) {
        irq = ffs(stat) - 1;
        handle_IRQ(irq_find_mapping(vic->domain, irq), regs);
        handled = 1;
    }

    return handled;
}

/*
 * Keep iterating over all registered VIC's until there are no pending
 * interrupts.
 */
asmlinkage void __exception_irq_entry gk_vic_handle_irq(struct pt_regs *regs)
{
    int i, handled;
    do {
        for (i = 0, handled = 0; i < gk_vic_id; ++i)
            handled |= gk_handle_one_vic(&gk_vic_decs[i], regs);
    } while (handled);
}

#else

void __init gk_init_vic(u32 num_vic)
{
    u32 vic[] = {~0, ~0, ~0};
    int irq;

printk("gk init vic...\n");
    /* initialize the VICs */
    for (irq = 0; irq < num_vic; irq++)
    {
        vic_init(vic_base_addr[irq], (NR_VIC_IRQ_SIZE * irq), vic[irq], 0);
    }
}

#endif

void __init gk_init_irq(void)
{

#ifndef CONFIG_ARM_VIC
    printk(">> gk init irq vic1...\n");
    /* Set VIC sense and event type for each entry
        * note: we initialize udc vbus irq type here */
    gk_irq_writel(VIC_SENSE_REG, 0x00000000);
    gk_irq_writel(VIC_BOTHEDGE_REG, 0x00000000);
    gk_irq_writel(VIC_EVENT_REG, 0x00000000);

#if (VIC_INSTANCES >= 2)
    printk(">> gk init irq vic2...\n");
    gk_irq_writel(VIC2_SENSE_REG, 0x00000000);
    gk_irq_writel(VIC2_BOTHEDGE_REG, 0x00000000);
    gk_irq_writel(VIC2_EVENT_REG, 0x00000000);
#endif

    /* Disable all IRQ */
    gk_irq_writel(VIC_INT_SEL_REG, 0x00000000);
    gk_irq_writel(VIC_INTEN_REG, 0x00000000);
    gk_irq_writel(VIC_INTEN_CLR_REG, 0xffffffff);
    gk_irq_writel(VIC_EDGE_CLR_REG, 0xffffffff);

#if (VIC_INSTANCES >= 2)
    gk_irq_writel(VIC2_INT_SEL_REG, 0x00000000);
    gk_irq_writel(VIC2_INTEN_REG, 0x00000000);
    gk_irq_writel(VIC2_INTEN_CLR_REG, 0xffffffff);
    gk_irq_writel(VIC2_EDGE_CLR_REG, 0xffffffff);
#endif
#endif

    gk_init_vic(VIC_INSTANCES);

}

