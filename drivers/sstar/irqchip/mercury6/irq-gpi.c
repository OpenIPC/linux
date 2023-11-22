/*
 * irq-gpi.c- Sigmastar
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
#include <asm/mach/irq.h>
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

#ifdef CONFIG_PM_SLEEP
/**
 * struct ss_gpi_irq_priv - private gpi interrupt data
 * @polarity:   fiq polarity
 */
struct ss_gpi_irq_priv
{
    U16 gpi_polarity[(GPI_FIQ_NUM + 15) >> 4];
    U16 gpic_polarity[(GPI_GPIC_NUM + 15) >> 4];
};

static struct ss_gpi_irq_priv gpi_irq_priv;
#endif

static int parent_gpi_hwirq = 0;

static void ss_gpi_irq_ack(struct irq_data *d)
{
    U16 gpi_irq;

    if (!d)
    {
        dump_stack();
        return;
    }
    gpi_irq = d->hwirq;
    pr_debug("[%s] hw:%d\n", __FUNCTION__, gpi_irq);

    if (gpi_irq >= 0 && gpi_irq < (GPI_FIQ_NUM + GPI_GPIC_NUM))
    {
        if (gpi_irq < GPI_GPIC_START)
        {
            SETREG16((BASE_REG_GPI_INT_PA + REG_ID_14 + (gpi_irq / 16) * 4), (1 << (gpi_irq % 16)));
            INREG16(BASE_REG_MAILBOX_PA); // read a register make ensure the previous write command was compeleted
        }
        else
        {
            SETREG16((BASE_REG_GPI_INT_PA + REG_ID_7A + ((gpi_irq - GPI_GPIC_START) / 16) * 4),
                     (1 << ((gpi_irq - GPI_GPIC_START) % 16)));
            INREG16(BASE_REG_MAILBOX_PA); // read a register make ensure the previous write command was compeleted
        }
    }
    else
    {
        pr_err("[%s] Unknown hwirq %lu\n", __func__, d->hwirq);
        return;
    }
}

static void ss_gpi_irq_mask(struct irq_data *d)
{
    U16 gpi_irq;

    gpi_irq = d->hwirq;
    pr_debug("[%s] hw:%d \n", __FUNCTION__, gpi_irq);

    if (gpi_irq >= 0 && gpi_irq < (GPI_FIQ_NUM + GPI_GPIC_NUM))
    {
        if (gpi_irq < GPI_GPIC_START)
        {
            SETREG16((BASE_REG_GPI_INT_PA + REG_ID_00 + (gpi_irq / 16) * 4), (1 << (gpi_irq % 16)));
            INREG16(BASE_REG_MAILBOX_PA); // read a register make ensure the previous write command was compeleted
        }
        else
        {
            SETREG16((BASE_REG_GPI_INT_PA + REG_ID_78 + ((gpi_irq - GPI_GPIC_START) / 16) * 4),
                     (1 << ((gpi_irq - GPI_GPIC_START) % 16)));
            INREG16(BASE_REG_MAILBOX_PA); // read a register make ensure the previous write command was compeleted
        }
    }
    else
    {
        pr_err("[%s] Unknown hwirq %lu\n", __func__, d->hwirq);
        return;
    }
}

static void ss_gpi_irq_unmask(struct irq_data *d)
{
    U16 gpi_irq;

    gpi_irq = d->hwirq;
    pr_debug("[%s] hw:%d \n", __FUNCTION__, gpi_irq);

    if (gpi_irq >= 0 && gpi_irq < (GPI_FIQ_NUM + GPI_GPIC_NUM))
    {
        if (gpi_irq < GPI_GPIC_START)
        {
            CLRREG16((BASE_REG_GPI_INT_PA + REG_ID_00 + (gpi_irq / 16) * 4), (1 << (gpi_irq % 16)));
        }
        else
        {
            CLRREG16((BASE_REG_GPI_INT_PA + REG_ID_78 + ((gpi_irq - GPI_GPIC_START) / 16) * 4),
                     (1 << ((gpi_irq - GPI_GPIC_START) % 16)));
        }
    }
    else
    {
        pr_err("[%s] Unknown hwirq %lu\n", __func__, d->hwirq);
        return;
    }
    CLRREG16(BASE_REG_INTRCTL_PA + REG_ID_57, BIT8);
    INREG16(BASE_REG_MAILBOX_PA); // read a register make ensure the previous write command was compeleted
}

static int ss_gpi_irq_set_type(struct irq_data *d, unsigned int type)
{
    U16 gpi_irq;
    pr_debug("%s %d type:0x%08x\n", __FUNCTION__, __LINE__, type);

    gpi_irq = d->hwirq;

    if (gpi_irq >= 0 && gpi_irq < (GPI_FIQ_NUM + GPI_GPIC_NUM))
    {
        switch (type)
        {
            case IRQ_TYPE_EDGE_FALLING:
                if (gpi_irq < GPI_GPIC_START)
                {
                    SETREG16((BASE_REG_GPI_INT_PA + REG_ID_1E + (gpi_irq / 16) * 4), (1 << (gpi_irq % 16)));
                }
                else
                {
                    SETREG16((BASE_REG_GPI_INT_PA + REG_ID_7B + ((gpi_irq - GPI_GPIC_START) / 16) * 4),
                             (1 << ((gpi_irq - GPI_GPIC_START) % 16)));
                }
                break;
            case IRQ_TYPE_EDGE_RISING:
                if (gpi_irq < GPI_GPIC_START)
                {
                    CLRREG16((BASE_REG_GPI_INT_PA + REG_ID_1E + (gpi_irq / 16) * 4), (1 << (gpi_irq % 16)));
                }
                else
                {
                    CLRREG16((BASE_REG_GPI_INT_PA + REG_ID_7B + ((gpi_irq - GPI_GPIC_START) / 16) * 4),
                             (1 << ((gpi_irq - GPI_GPIC_START) % 16)));
                }
                break;
            default:
                return -EINVAL;
        }
        // prevent the very 1st unexpected trigger right after irq_request()
        // http://mantis.sigmastar.com.tw/view.php?id=1688845
        ss_gpi_irq_ack(d);
    }
    else
    {
        pr_err("[%s] Unknown hwirq %lu\n", __func__, d->hwirq);
        return -EINVAL;
    }

    return 0;
}

struct irq_chip ss_gpi_intc_irqchip = {
    .name         = "MS_GPI_INTC",
    .irq_ack      = ss_gpi_irq_ack,
    .irq_eoi      = ss_gpi_irq_ack,
    .irq_mask     = ss_gpi_irq_mask,
    .irq_unmask   = ss_gpi_irq_unmask,
    .irq_set_type = ss_gpi_irq_set_type,
};
EXPORT_SYMBOL(ss_gpi_intc_irqchip);

static void ss_handle_cascade_gpi(struct irq_desc *desc)
{
    unsigned int       cascade_irq = 0xFFFFFFFF, i, j;
    unsigned int       virq        = 0xFFFFFFFF;
    struct irq_chip *  chip        = irq_desc_get_chip(desc);
    struct irq_domain *domain      = irq_desc_get_handler_data(desc);
    unsigned int       final_status;

    if (!domain)
    {
        printk("[%s] err %d \n", __FUNCTION__, __LINE__);
        goto exit;
    }

    for (j = 0; j <= GPI_FIQ_NUM / 16; j++)
    {
        final_status = INREG16(BASE_REG_GPI_INT_PA + REG_ID_36 + j * 4);
        for (i = 0; i < 16 && final_status != 0; i++)
        {
            if (0 != (final_status & (1 << i)))
            {
                cascade_irq = i + j * 16;
                goto handle_int;
            }
        }
    }

    for (j = 0; j <= GPI_GPIC_NUM / 16; j++)
    {
        final_status = INREG16(BASE_REG_GPI_INT_PA + REG_ID_7C + j * 4);
        for (i = 0; i < 16 && final_status != 0; i++)
        {
            if (0 != (final_status & (1 << i)))
            {
                cascade_irq = i + j * 16;
                cascade_irq += GPI_GPIC_START;
                goto handle_int;
            }
        }
    }

    if (0xFFFFFFFF == cascade_irq)
    {
        pr_err("[%s:%d] error final_status:%d 0x%04X virq:%d\n", __FUNCTION__, __LINE__, cascade_irq, final_status,
               virq);
        panic("[%s] error %d \n", __FUNCTION__, __LINE__);
        chained_irq_exit(chip, desc);
        goto exit;
    }

handle_int:
    virq = irq_find_mapping(domain, cascade_irq);
    if (!virq)
    {
        printk("[%s] err %d cascade_irq:%d\n", __FUNCTION__, __LINE__, cascade_irq);
        goto exit;
    }
    pr_debug("%s %d final_status:%d 0x%04X virq:%d\n", __FUNCTION__, __LINE__, cascade_irq, final_status, virq);
    chained_irq_enter(chip, desc);
    generic_handle_irq(virq);

exit:
    chained_irq_exit(chip, desc);
}

static int ss_gpi_intc_domain_translate(struct irq_domain *domain, struct irq_fwspec *fwspec, unsigned long *hwirq,
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

static int ss_gpi_intc_domain_alloc(struct irq_domain *domain, unsigned int virq, unsigned int nr_irqs, void *data)
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
        irq_domain_set_info(domain, virq + i, hwirq + i, &ss_gpi_intc_irqchip, NULL, handle_edge_irq, NULL, NULL);
        pr_debug("[%s] hw:%d -> v:%d\n", __FUNCTION__, (unsigned int)hwirq + i, virq + i);
    }

    parent_fwspec.fwnode      = domain->parent->fwnode;
    parent_fwspec.param_count = 3;
    parent_fwspec.param[0]    = GIC_SPI;
    parent_fwspec.param[1]    = parent_gpi_hwirq;
    parent_fwspec.param[2]    = IRQ_TYPE_EDGE_BOTH;

    return irq_domain_alloc_irqs_parent(domain, virq, nr_irqs, &parent_fwspec);
}

static void ss_gpi_intc_domain_free(struct irq_domain *domain, unsigned int virq, unsigned int nr_irqs)
{
    unsigned int i;

    for (i = 0; i < nr_irqs; i++)
    {
        struct irq_data *d = irq_domain_get_irq_data(domain, virq + i);
        irq_domain_reset_irq_data(d);
    }
}

struct irq_domain_ops ss_gpi_intc_domain_ops = {
    .translate = ss_gpi_intc_domain_translate,
    .alloc     = ss_gpi_intc_domain_alloc,
    .free      = ss_gpi_intc_domain_free,
};

#ifdef CONFIG_PM_SLEEP
static int ss_gpi_intc_suspend(void)
{
    unsigned int i, num;

    num = (GPI_FIQ_NUM + 15) >> 4;
    for (i = 0; i < num; i++)
    {
        gpi_irq_priv.gpi_polarity[i] = INREG16(BASE_REG_GPI_INT_PA + REG_ID_1E + (i << 2));
    }

    num = (GPI_GPIC_NUM + 15) >> 4;
    for (i = 0; i < num; i++)
    {
        gpi_irq_priv.gpic_polarity[i] = INREG16(BASE_REG_GPI_INT_PA + REG_ID_7B + (i << 2));
    }

    pr_debug("ss_gpi_intc_suspend\n\n");
    return 0;
}

static void ss_gpi_intc_resume(void)
{
    unsigned int i, num;

    num = (GPI_FIQ_NUM + 15) >> 4;
    for (i = 0; i < num; i++)
    {
        OUTREG16(BASE_REG_GPI_INT_PA + REG_ID_1E + (i << 2), gpi_irq_priv.gpi_polarity[i]);
    }

    num = (GPI_GPIC_NUM + 15) >> 4;
    for (i = 0; i < num; i++)
    {
        OUTREG16(BASE_REG_GPI_INT_PA + REG_ID_7B + (i << 2), gpi_irq_priv.gpic_polarity[i]);
    }

    pr_debug("ss_gpi_intc_resume\n\n");
}

struct syscore_ops ss_gpi_intc_syscore_ops = {
    .suspend = ss_gpi_intc_suspend,
    .resume  = ss_gpi_intc_resume,
};
#endif

static int __init ss_init_gpi_intc(struct device_node *np, struct device_node *interrupt_parent)
{
    int                    parent_gpi_virtirq = 0;
    struct of_phandle_args irq_handle         = {0};
    struct irq_domain *    parent_domain, *ss_gpi_irq_domain;

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

    ss_gpi_irq_domain = irq_domain_add_hierarchy(parent_domain, 0,
                                                 // PMSLEEP_FIQ_NR, //76
                                                 GPI_FIQ_NUM + GPI_GPIC_NUM, // 94
                                                 np, &ss_gpi_intc_domain_ops, NULL);

    if (!ss_gpi_irq_domain)
    {
        pr_err("%s: %s allocat domain fail\n", __func__, np->name);
        return -ENOMEM;
    }

    if (of_irq_parse_one(np, 0, &irq_handle))
        pr_err("%s: %s of_irq_parse_one fail\n", __func__, np->name);

    parent_gpi_hwirq   = irq_handle.args[1];
    parent_gpi_virtirq = irq_create_of_mapping(&irq_handle);

    if (!parent_gpi_virtirq)
    {
        pr_err("Get irq err from DTS\n");
        return -EPROBE_DEFER;
    }

    irq_set_chained_handler_and_data(parent_gpi_virtirq, ss_handle_cascade_gpi, ss_gpi_irq_domain);
#ifdef CONFIG_PM_SLEEP
    register_syscore_ops(&ss_gpi_intc_syscore_ops);
#endif
    return 0;
}

IRQCHIP_DECLARE(ss_gpi_intc, "sstar,gpi-intc", ss_init_gpi_intc);
