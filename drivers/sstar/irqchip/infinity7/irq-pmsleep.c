/*
 * irq-pmsleep.c- Sigmastar
 *
 * Copyright (c) [2019~2020] SigmaStar Technology.
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License version 2 for more details.
 *
 */
#include <linux/init.h>
#include <linux/io.h>
#include <linux/irqchip/chained_irq.h>
#include <linux/irqdomain.h>
#include <linux/of.h>
#include <linux/syscore_ops.h>
#ifdef CONFIG_ARM64
#include "irq.h"
#else
#include <asm/mach/irq.h>
#endif
#include <linux/irqchip/arm-gic.h>
#include <linux/irqchip.h>
#include <linux/irqdesc.h>

#include <dt-bindings/interrupt-controller/arm-gic.h>

#include "irqs.h"
#include "registers.h"

#include "_ms_private.h"
#include "ms_platform.h"
#include "ms_types.h"

#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/interrupt.h>

#define PMGPIO_OEN              BIT0
#define PMGPIO_OUTPUT           BIT1
#define PMGPIO_INPUT            BIT2
#define PMGPIO_FIQ_MASK         BIT4
#define PMGPIO_FIQ_FROCE        BIT5
#define PMGPIO_FIQ_CLEAR        BIT6
#define PMGPIO_FIQ_POLARITY     BIT7
#define PMGPIO_FIQ_FINAL_STATUS BIT8
#define PMGPIO_FIQ_RAW_STATUS   BIT9

#ifdef CONFIG_PM_SLEEP
#define PM_FIQ_NUM (INT_PMSLEEP_SAR_GPIO5 - PMSLEEP_FIQ_START + 1)

/**
 * struct ms_pm_irq_priv - private pm interrupt data
 * @fiq_flags:      Flags of each pm-gpio fiq
 * @irq_polarity:   Polarity of pm-sleep irq
 */
struct ms_pm_irq_priv
{
    U16 fiq_flags[PM_FIQ_NUM];
    U8  irq_polarity;
};

static struct ms_pm_irq_priv pm_irq_priv;
#endif
static int parent_pm_hwirq = 0;

static void ms_pm_irq_ack(struct irq_data *d)
{
    U16 pmsleep_fiq;

    pmsleep_fiq = d->hwirq;
    pr_debug("[%s] hw:%d \n", __FUNCTION__, pmsleep_fiq);

    if (pmsleep_fiq < PMSLEEP_FIQ_END)
    {
        SETREG16(BASE_REG_PMGPIO_PA + ((pmsleep_fiq - PMSLEEP_FIQ_START) << 2), PMGPIO_FIQ_CLEAR);
        INREG16(BASE_REG_MAILBOX_PA); // read a register make ensure the previous write command was compeleted
    }
    else
    {
        pr_err("[%s] Unknown hwirq %lu\n", __func__, d->hwirq);
        return;
    }
}

static void ms_pm_irq_eoi(struct irq_data *d)
{
    U16 pmsleep_fiq;

    pmsleep_fiq = d->hwirq;
    pr_debug("[%s] hw:%d \n", __FUNCTION__, pmsleep_fiq);

    if (pmsleep_fiq < PMSLEEP_FIQ_END)
    {
        SETREG16(BASE_REG_PMGPIO_PA + ((pmsleep_fiq - PMSLEEP_FIQ_START) << 2), PMGPIO_FIQ_CLEAR);
        INREG16(BASE_REG_MAILBOX_PA); // read a register make ensure the previous write command was compeleted
    }
    else
    {
        pr_err("[%s] Unknown hwirq %lu\n", __func__, d->hwirq);
        return;
    }
}

static void ms_pm_irq_mask(struct irq_data *d)
{
    U16 pmsleep_fiq;

    pmsleep_fiq = d->hwirq;
    pr_debug("[%s] hw:%d \n", __FUNCTION__, pmsleep_fiq);

    if (pmsleep_fiq < PMSLEEP_FIQ_END)
    {
        SETREG16(BASE_REG_PMGPIO_PA + ((pmsleep_fiq - PMSLEEP_FIQ_START) << 2), PMGPIO_FIQ_MASK);
        INREG16(BASE_REG_MAILBOX_PA); // read a register make ensure the previous write command was compeleted
    }
    else if (pmsleep_fiq < PMSLEEP_IRQ_END)
    {
        SETREG8(BASE_REG_PMSLEEP_PA + REG_ID_08, 1 << (pmsleep_fiq - PMSLEEP_IRQ_START));
        INREG16(BASE_REG_MAILBOX_PA); // read a register make ensure the previous write command was compeleted
    }
    else
    {
        pr_err("[%s] Unknown hwirq %lu\n", __func__, d->hwirq);
        return;
    }
}

static void ms_pm_irq_unmask(struct irq_data *d)
{
    U16 pmsleep_fiq;

    pmsleep_fiq = d->hwirq;
    pr_debug("[%s] hw:%d \n", __FUNCTION__, pmsleep_fiq);

    if (pmsleep_fiq < PMSLEEP_FIQ_END)
    {
        CLRREG16(BASE_REG_PMGPIO_PA + ((pmsleep_fiq - PMSLEEP_FIQ_START) << 2), PMGPIO_FIQ_MASK);
    }
    else if (pmsleep_fiq < PMSLEEP_IRQ_END)
    {
        CLRREG8(BASE_REG_PMSLEEP_PA + REG_ID_08, 1 << (pmsleep_fiq - PMSLEEP_IRQ_START));
    }
    else
    {
        pr_err("[%s] Unknown hwirq %lu\n", __func__, d->hwirq);
        return;
    }

    CLRREG16(BASE_REG_INTRCTL_PA + REG_ID_54, BIT2);
    INREG16(BASE_REG_MAILBOX_PA); // read a register make ensure the previous write command was compeleted
}

static int ms_pm_irq_set_type(struct irq_data *d, unsigned int type)
{
    U16 pmsleep_fiq;
    pr_debug("%s %d type:0x%08x\n", __FUNCTION__, __LINE__, type);

    pmsleep_fiq = d->hwirq;

    if (pmsleep_fiq < PMSLEEP_FIQ_END)
    {
        switch (type)
        {
            case IRQ_TYPE_EDGE_FALLING:
            case IRQ_TYPE_LEVEL_LOW:
                SETREG16(BASE_REG_PMGPIO_PA + ((pmsleep_fiq - PMSLEEP_FIQ_START) << 2), PMGPIO_FIQ_POLARITY);
                break;
            case IRQ_TYPE_EDGE_RISING:
            case IRQ_TYPE_LEVEL_HIGH:
                CLRREG16(BASE_REG_PMGPIO_PA + ((pmsleep_fiq - PMSLEEP_FIQ_START) << 2), PMGPIO_FIQ_POLARITY);
                break;
            default:
                return -EINVAL;
        }
    }
    else if (pmsleep_fiq < PMSLEEP_IRQ_END)
    {
        switch (type)
        {
            case IRQ_TYPE_EDGE_FALLING:
            case IRQ_TYPE_LEVEL_LOW:
                SETREG8(BASE_REG_PMSLEEP_PA + REG_ID_09, 1 << (pmsleep_fiq - PMSLEEP_IRQ_START));
                break;
            case IRQ_TYPE_EDGE_RISING:
            case IRQ_TYPE_LEVEL_HIGH:
                CLRREG8(BASE_REG_PMSLEEP_PA + REG_ID_09, 1 << (pmsleep_fiq - PMSLEEP_IRQ_START));
                break;
            default:
                return -EINVAL;
        }
    }
    else
    {
        pr_err("[%s] Unknown hwirq %lu\n", __func__, d->hwirq);
        return -EINVAL;
    }

    return 0;
}

struct irq_chip ms_pm_intc_irqchip = {
    .name         = "MS_PM_INTC",
    .irq_ack      = ms_pm_irq_ack,
    .irq_eoi      = ms_pm_irq_eoi,
    .irq_mask     = ms_pm_irq_mask,
    .irq_unmask   = ms_pm_irq_unmask,
    .irq_set_type = ms_pm_irq_set_type,
};
EXPORT_SYMBOL(ms_pm_intc_irqchip);

static DEFINE_SPINLOCK(ss_irq_controller_lock);

static void ms_handle_cascade_pm_irq(struct irq_desc *desc)
{
    unsigned int       cascade_irq = 0xFFFFFFFF, i;
    unsigned int       virq        = 0;
    struct irq_chip *  chip        = irq_desc_get_chip(desc);
    struct irq_domain *domain      = irq_desc_get_handler_data(desc);
    unsigned int       final_status;

    if (!domain)
    {
        printk("[%s] err %d \n", __FUNCTION__, __LINE__);
        goto exit;
    }

    spin_lock(&ss_irq_controller_lock);
    for (i = PMSLEEP_FIQ_START; i < PMSLEEP_FIQ_END; i++)
    {
        final_status = INREG16(BASE_REG_PMGPIO_PA + (i - PMSLEEP_FIQ_START) * 4);
        if (final_status & PMGPIO_FIQ_FINAL_STATUS)
        {
            cascade_irq = i;
            pr_debug("[%s] Get hwirq:%d, Reg:0x%04x\n", __FUNCTION__, cascade_irq, final_status);
            break;
        }
    }
    spin_unlock(&ss_irq_controller_lock);

    if (0xFFFFFFFF == cascade_irq)
    {
        pr_err("[%s:%d] error final_status:%d 0x%04X virq:%d\n", __FUNCTION__, __LINE__, cascade_irq, final_status,
               virq);
        panic("%s(%d) invalid cascade_irq: %d\n", __FUNCTION__, __LINE__, cascade_irq);
        chained_irq_exit(chip, desc);
        goto exit;
    }

    virq = irq_find_mapping(domain, cascade_irq);
    if (!virq)
    {
        printk("[%s] err %d \n", __FUNCTION__, __LINE__);
        goto exit;
    }
    pr_debug("%s %d final_status:%d 0x%04X virq:%d\n", __FUNCTION__, __LINE__, cascade_irq, final_status, virq);
    chained_irq_enter(chip, desc);
    generic_handle_irq(virq);

exit:
    chained_irq_exit(chip, desc);
}

static int ms_pm_intc_domain_translate(struct irq_domain *domain, struct irq_fwspec *fwspec, unsigned long *hwirq,
                                       unsigned int *type)
{
    if (is_of_node(fwspec->fwnode))
    {
        if (fwspec->param_count != 1)
            return -EINVAL;
        *hwirq = fwspec->param[0];
        return 0;
    }

    return -EINVAL;
}

static int ms_pm_intc_domain_alloc(struct irq_domain *domain, unsigned int virq, unsigned int nr_irqs, void *data)
{
    struct irq_fwspec *fwspec = data;
    struct irq_fwspec  parent_fwspec;
    irq_hw_number_t    hwirq;
    unsigned int       i;

    if (fwspec->param_count != 1)
        return -EINVAL;

    hwirq = fwspec->param[0];

    for (i = 0; i < nr_irqs; i++)
    {
        irq_domain_set_info(domain, virq + i, hwirq + i, &ms_pm_intc_irqchip, NULL, handle_fasteoi_irq, NULL, NULL);
        pr_err("[MS_PM_INTC] hw:%d -> v:%d\n", (unsigned int)hwirq + i, virq + i);
    }

    parent_fwspec.fwnode      = domain->parent->fwnode;
    parent_fwspec.param_count = 3;
    parent_fwspec.param[0]    = GIC_SPI;
    parent_fwspec.param[1]    = parent_pm_hwirq;
    parent_fwspec.param[2]    = IRQ_TYPE_EDGE_BOTH;

    return irq_domain_alloc_irqs_parent(domain, virq, nr_irqs, &parent_fwspec);
}

static void ms_pm_intc_domain_free(struct irq_domain *domain, unsigned int virq, unsigned int nr_irqs)
{
    unsigned int i;

    for (i = 0; i < nr_irqs; i++)
    {
        struct irq_data *d = irq_domain_get_irq_data(domain, virq + i);
        irq_domain_reset_irq_data(d);
    }
}

struct irq_domain_ops ms_pm_intc_domain_ops = {
    .translate = ms_pm_intc_domain_translate,
    .alloc     = ms_pm_intc_domain_alloc,
    .free      = ms_pm_intc_domain_free,
};

#ifdef CONFIG_PM_SLEEP
static int ms_pm_intc_suspend(void)
{
    unsigned int i;

    for (i = 0; i < PM_FIQ_NUM; i++)
    {
        pm_irq_priv.fiq_flags[i] = INREG16(BASE_REG_PMGPIO_PA + (i << 2));
        pm_irq_priv.fiq_flags[i] |= PMGPIO_FIQ_MASK; // always set mask here, unmask will be set by irq/pm.c
    }
    pm_irq_priv.irq_polarity = INREG8(BASE_REG_PMSLEEP_PA + REG_ID_09);

    pr_debug("ms_pm_intc_suspend\n\n");
    return 0;
}

static void ms_pm_intc_resume(void)
{
    unsigned int i;

    for (i = 0; i < PM_FIQ_NUM; i++)
    {
        OUTREG16(BASE_REG_PMGPIO_PA + (i << 2), pm_irq_priv.fiq_flags[i]);
    }
    OUTREG8(BASE_REG_PMSLEEP_PA + REG_ID_09, pm_irq_priv.irq_polarity);
    pr_debug("ms_pm_intc_resume\n\n");
}

struct syscore_ops ms_pm_intc_syscore_ops = {
    .suspend = ms_pm_intc_suspend,
    .resume  = ms_pm_intc_resume,
};
#endif

static int __init ms_init_pm_intc(struct device_node *np, struct device_node *interrupt_parent)
{
    int                    parent_pm_virtirq = 0;
    struct of_phandle_args irq_handle        = {0};
    struct irq_domain *    parent_domain, *ms_pm_irq_domain;

    if (!interrupt_parent)
    {
        pr_err("%s: %s no parent\n", __func__, np->name);
        return -ENODEV;
    }

    pr_err("%s: np->name=%s, parent=%s\n", __func__, np->name, interrupt_parent->name);

    parent_domain = irq_find_host(interrupt_parent);
    if (!parent_domain)
    {
        pr_err("%s: %s unable to obtain parent domain\n", __func__, np->name);
        return -ENXIO;
    }

    ms_pm_irq_domain = irq_domain_add_hierarchy(parent_domain, 0, PMSLEEP_FIQ_NR, np, &ms_pm_intc_domain_ops, NULL);

    if (!ms_pm_irq_domain)
    {
        pr_err("%s: %s allocat domain fail\n", __func__, np->name);
        return -ENOMEM;
    }

    if (of_irq_parse_one(np, 0, &irq_handle))
        pr_err("%s: %s of_irq_parse_one fail\n", __func__, np->name);

    parent_pm_hwirq   = irq_handle.args[1];
    parent_pm_virtirq = irq_create_of_mapping(&irq_handle);

    if (!parent_pm_virtirq)
    {
        pr_err("Get irq err from DTS\n");
        return -EPROBE_DEFER;
    }

    irq_set_chained_handler_and_data(parent_pm_virtirq, ms_handle_cascade_pm_irq, ms_pm_irq_domain);

#ifdef CONFIG_PM_SLEEP
    register_syscore_ops(&ms_pm_intc_syscore_ops);
#endif
    return 0;
}

IRQCHIP_DECLARE(ms_pm_intc, "sstar,pm-intc", ms_init_pm_intc);
