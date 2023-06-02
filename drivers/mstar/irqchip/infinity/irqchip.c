#include <linux/init.h>
#include <linux/irqchip/arm-gic.h>
#include <linux/irqchip.h>
#include <linux/io.h>
#include <linux/irqchip/chained_irq.h>
#include <linux/irqdomain.h>
#include <linux/of.h>
#include <linux/syscore_ops.h>
#include <asm/mach/irq.h>
#include <irqchip.h>

#include "infinity/irqs.h"
#include "infinity/registers.h"

#include "_ms_private.h"
#include "ms_platform.h"
#include "ms_types.h"


static DEFINE_SPINLOCK(infinity_irq_controller_lock);


/*         _ _ _ _ _ _ _ _ _ _                  */
/*        |                   |                 */
/*        |  PM_SLEEP_IRQ(32) |                 */
/*        |_ _ _ _ _ _ _ _ _ _|                 */
/*        |                   |                 */
/*        |    MS_FIQ (32)    |                 */
/*        |_ _ _ _ _ _ _ _ _ _| ms_fiq          */
/*        |                   |                 */
/*        |    MS_IRQ (64)    |                 */
/*        |_ _ _ _ _ _ _ _ _ _| ms_irq          */
/*        |                   |                 */
/*        |  ARM_INTERNAL(32) |                 */
/*        |_ _ _ _ _ _ _ _ _ _| gic_spi         */
/*        |                   |                 */
/*        |      PPI (16)     |                 */
/*        |_ _ _ _ _ _ _ _ _ _|                 */
/*        |                   |                 */
/*        |      SGI (16)     |                 */
/*        |_ _ _ _ _ _ _ _ _ _|                 */
/*                                              */


static void ms_pm_sleep_irq_ack(struct irq_data *d)
{
    U16 pmsleep_fiq;

    pmsleep_fiq = d->hwirq;

    if(pmsleep_fiq == INT_PMSLEEP_IR)
    {
        SETREG16(BASE_REG_PMSLEEP_PA + REG_ID_02, BIT0);
    }
    else if(pmsleep_fiq == INT_PMSLEEP_DVI_CK_DET)
    {
        SETREG16(BASE_REG_PMSLEEP_PA + REG_ID_02, BIT1);
    }
    else if(pmsleep_fiq < PMSLEEP_FIQ_END)
    {
        pmsleep_fiq -= 2/*first 2 pm_sleep_irqs are not gpio*/;
        SETREG16(BASE_REG_PMGPIO_PA + (pmsleep_fiq << 2), BIT6);
    }
}

static void ms_pm_sleep_irq_mask(struct irq_data *d)
{
    U16 pmsleep_fiq;

    pmsleep_fiq = d->hwirq;

    if(pmsleep_fiq == INT_PMSLEEP_IR)
    {
        SETREG16(BASE_REG_PMSLEEP_PA + REG_ID_00, BIT0);
    }
    else if(pmsleep_fiq == INT_PMSLEEP_DVI_CK_DET)
    {
        SETREG16(BASE_REG_PMSLEEP_PA + REG_ID_00, BIT1);
    }
    else if(pmsleep_fiq < PMSLEEP_FIQ_END)
    {
        pmsleep_fiq -= 2/*first 2 pm_sleep_irqs are not gpio*/;
        SETREG16(BASE_REG_PMGPIO_PA + (pmsleep_fiq << 2), BIT4);
    }
}

static void ms_pm_sleep_irq_unmask(struct irq_data *d)
{
    U16 pmsleep_fiq;

    pmsleep_fiq = d->hwirq;


    if(pmsleep_fiq == INT_PMSLEEP_IR)
    {
        CLRREG16(BASE_REG_PMSLEEP_PA + REG_ID_00, BIT0);
    }
    else if(pmsleep_fiq == INT_PMSLEEP_DVI_CK_DET)
    {
        CLRREG16(BASE_REG_PMSLEEP_PA + REG_ID_00, BIT1);
    }
    else if(pmsleep_fiq < PMSLEEP_FIQ_END)
    {
        pmsleep_fiq -= 2/*first 2 pm_sleep_irqs are not gpio*/;
        CLRREG16(BASE_REG_PMGPIO_PA + (pmsleep_fiq << 2), BIT4);
    }
    else
        return;

    CLRREG16(BASE_REG_INTRCTL_PA + REG_ID_54, BIT2);
}

static int ms_pm_sleep_irq_set_type(struct irq_data *d, unsigned int type)
{
    U16 pmsleep_fiq;

    pmsleep_fiq = d->hwirq;

    if(pmsleep_fiq == INT_PMSLEEP_IR)
    {
        if(type)
            SETREG16(BASE_REG_PMSLEEP_PA + REG_ID_03, BIT0);
        else
            CLRREG16(BASE_REG_PMSLEEP_PA + REG_ID_03, BIT0);
    }
    else if(pmsleep_fiq == INT_PMSLEEP_DVI_CK_DET)
    {
        if(type)
            SETREG16(BASE_REG_PMSLEEP_PA + REG_ID_03, BIT1);
        else
            CLRREG16(BASE_REG_PMSLEEP_PA + REG_ID_03, BIT1);
    }
    else if(pmsleep_fiq < PMSLEEP_FIQ_END)
    {
        pmsleep_fiq -= 2/*first 2 pm_sleep_irqs are not gpio*/;
        if(type)
            SETREG16(BASE_REG_PMGPIO_PA + (pmsleep_fiq << 2), BIT7);
        else
            CLRREG16(BASE_REG_PMGPIO_PA + (pmsleep_fiq << 2), BIT7);
    }
    return 0;
}

static void ms_eoi_irq(struct irq_data *d)
{
    s16 ms_fiq;

    ms_fiq = d->hwirq - GIC_HWIRQ_MS_START - GIC_SPI_MS_IRQ_NR;

    /* NOTE: only clear if it is FIQ */
    if( d->hwirq < GIC_HWIRQ_MS_START )
    {
        return;
    }
    else if( ms_fiq >= 0 && ms_fiq < GIC_SPI_MS_FIQ_NR )
    {
        SETREG16( (BASE_REG_INTRCTL_PA + REG_ID_4C + (ms_fiq/16)*4 ) , (1 << (ms_fiq%16)) );
    }
    else if( ms_fiq >= GIC_SPI_MS_FIQ_NR )
    {
        pr_err("[ms_irq_eoi] Unknown ID %lu from GIC\n", d->hwirq);
    }
}

static void ms_mask_irq(struct irq_data *d)
{
    s16 ms_irq;
    s16 ms_fiq;

    ms_irq = d->hwirq - GIC_HWIRQ_MS_START;
    ms_fiq = d->hwirq - GIC_HWIRQ_MS_START - GIC_SPI_MS_IRQ_NR;

    if( d->hwirq < GIC_HWIRQ_MS_START )
    {
        return;
    }
    else if( ms_fiq >= 0 && ms_fiq < GIC_SPI_MS_FIQ_NR )
    {
        SETREG16( (BASE_REG_INTRCTL_PA + REG_ID_44 + (ms_fiq/16)*4 ) , (1 << (ms_fiq%16)) );
    }
    else if( ms_irq >=0 && ms_irq < GIC_SPI_MS_IRQ_NR )
    {
        SETREG16( (BASE_REG_INTRCTL_PA + REG_ID_54 + (ms_irq/16)*4 ) , (1 << (ms_irq%16)) );
    }
    else
    {
        pr_err("[ms_irq_mask] Unknown hwirq %lu from GIC\n", d->hwirq);
    }
}

static void ms_unmask_irq(struct irq_data *d)
{
    s16 ms_irq;
    s16 ms_fiq;

    ms_irq = d->hwirq - GIC_HWIRQ_MS_START;
    ms_fiq = d->hwirq - GIC_HWIRQ_MS_START - GIC_SPI_MS_IRQ_NR;

    if( d->hwirq < GIC_HWIRQ_MS_START )
    {
        return;
    }
    else if( ms_fiq >= 0 && ms_fiq < GIC_SPI_MS_FIQ_NR )
    {
        CLRREG16( (BASE_REG_INTRCTL_PA + REG_ID_44 + (ms_fiq/16)*4 ) , (1 << (ms_fiq%16)) );
    }
    else if( ms_irq >=0 && ms_irq < GIC_SPI_MS_IRQ_NR )
    {
        CLRREG16( (BASE_REG_INTRCTL_PA + REG_ID_54 + (ms_irq/16)*4 ) , (1 << (ms_irq%16)) );
    }
    else
    {
        pr_err("[ms_irq_unmask] Unknown hwirq %lu from GIC\n", d->hwirq);
    }
}

static void set_irq_polarity(struct irq_data *d, bool pol)
{
    s16 ms_irq;
    s16 ms_fiq;

    ms_irq = d->hwirq - GIC_HWIRQ_MS_START;
    ms_fiq = d->hwirq - GIC_HWIRQ_MS_START - GIC_SPI_MS_IRQ_NR;

    if( d->hwirq < GIC_HWIRQ_MS_START )
    {
        return;
    }
    else if( ms_fiq >= 0 && ms_fiq < GIC_SPI_MS_FIQ_NR )
    {
        if (pol == true)
            SETREG16( (BASE_REG_INTRCTL_PA + REG_ID_48 + (ms_fiq/16)*4 ) , (1 << (ms_fiq%16)) );
        else
            CLRREG16( (BASE_REG_INTRCTL_PA + REG_ID_48 + (ms_fiq/16)*4 ) , (1 << (ms_fiq%16)) );
    }
    else if( ms_irq >=0 && ms_irq < GIC_SPI_MS_IRQ_NR )
    {
        if (pol == true)
            SETREG16( (BASE_REG_INTRCTL_PA + REG_ID_58 + (ms_irq/16)*4 ) , (1 << (ms_irq%16)) );
        else
            CLRREG16( (BASE_REG_INTRCTL_PA + REG_ID_58 + (ms_irq/16)*4 ) , (1 << (ms_irq%16)) );
    }
    else
    {
        pr_err("[ms_irq_set_polarity] Unknown hwirq %lu from GIC\n", d->hwirq);
    }
}

void ms_mask_irq_all(void)
{
    /* mask FIQ 0 - 31 */
    OUTREG16( (BASE_REG_INTRCTL_PA + REG_ID_44) , 0xFFFF );
    OUTREG16( (BASE_REG_INTRCTL_PA + REG_ID_45) , 0xFFFF );

    /* mask IRQ 0 - 63 */
    OUTREG16( (BASE_REG_INTRCTL_PA + REG_ID_54) , 0xFFFF );
    OUTREG16( (BASE_REG_INTRCTL_PA + REG_ID_55) , 0xFFFF );
    OUTREG16( (BASE_REG_INTRCTL_PA + REG_ID_56) , 0xFFFF );
    OUTREG16( (BASE_REG_INTRCTL_PA + REG_ID_57) , 0xFFFF );
}

void ms_unmask_irq_all(void)
{
    /* unmask FIQ 0 - 31 */
    OUTREG16( (BASE_REG_INTRCTL_PA + REG_ID_44) , 0 );
    OUTREG16( (BASE_REG_INTRCTL_PA + REG_ID_45) , 0 );

    /* unmask IRQ 0 - 63 */
    OUTREG16( (BASE_REG_INTRCTL_PA + REG_ID_54) , 0 );
    OUTREG16( (BASE_REG_INTRCTL_PA + REG_ID_55) , 0 );
    OUTREG16( (BASE_REG_INTRCTL_PA + REG_ID_56) , 0 );
    OUTREG16( (BASE_REG_INTRCTL_PA + REG_ID_57) , 0 );
}

static int ms_set_type_irq(struct irq_data *data, unsigned int flow_type)
{

    if(flow_type & (IRQ_TYPE_EDGE_FALLING|IRQ_TYPE_LEVEL_HIGH))
    {
        set_irq_polarity(data,0);
    }
    else
    {
        set_irq_polarity(data,1);
    }

    return 0;
}

static void ms_handle_cascade_pm_irq(unsigned int irq, struct irq_desc *desc)
{
    unsigned int cascade_irq = 0xFFFFFFFF, i;
    struct irq_chip *chip = irq_get_chip(irq);
    unsigned int final_status = INREG16(BASE_REG_PMSLEEP_PA + REG_ID_04) | (INREG16(BASE_REG_PMSLEEP_PA + REG_ID_05)<<16);

    chained_irq_enter(chip, desc);

    spin_lock(&infinity_irq_controller_lock);
    {
        for(i=0;i<32;i++)
        {
            if(0 !=(final_status & (1<<i)))
            {
                cascade_irq = i + (GIC_HWIRQ_MS_START+GIC_SPI_MS_IRQ_NR+GIC_SPI_MS_FIQ_NR);
                break;
            }
        }
    }
    spin_unlock(&infinity_irq_controller_lock);

    if(0xFFFFFFFF==cascade_irq)
        BUG();

    generic_handle_irq(cascade_irq);

    chained_irq_exit(chip, desc);
}

struct irq_chip ms_pm_sleep_irqchip = {
	.name = "PMSLEEP",
	.irq_ack = ms_pm_sleep_irq_ack,
	.irq_mask = ms_pm_sleep_irq_mask,
	.irq_unmask = ms_pm_sleep_irq_unmask,
    .irq_set_type = ms_pm_sleep_irq_set_type,
};
EXPORT_SYMBOL(ms_pm_sleep_irqchip);

static int pmsleep_irq_domain_map(struct irq_domain *d, unsigned int irq, irq_hw_number_t hw)
{
    irq_set_chip_and_handler(irq, &ms_pm_sleep_irqchip, handle_level_irq);
    set_irq_flags(irq, IRQF_VALID | IRQF_PROBE);
    return 0;
}

struct irq_domain_ops ms_irq_domain_ops = {
    .map = pmsleep_irq_domain_map,
	.xlate = irq_domain_xlate_onecell,
};


static int ms_irqchip_suspend(void)
{
    pr_debug("\nms_irqchip_suspend\n\n");
    return 0;
}

static void ms_irqchip_resume(void)
{
    pr_debug("\nms_irqchip_resume\n\n");

    //Patch for disable bypass IRQ/FIQ
    {
        u32 bypass = 0;
        bypass = INREG32(GIC_PHYS + 0x2000 + GIC_CPU_CTRL);
        bypass |= GICC_DIS_BYPASS_MASK;
        OUTREG32(GIC_PHYS + 0x2000 + GIC_CPU_CTRL, bypass | GICC_ENABLE);
    }
}

struct syscore_ops ms_irq_syscore_ops = {
    .suspend = ms_irqchip_suspend,
    .resume = ms_irqchip_resume,
};

static int __init infinity_init_irqchip(struct device_node *np, struct device_node *interrupt_parent)
{
    int hwirq_base = 0;
    int virq_base;
    struct irq_domain *infinity_irq_domain;

    //hook MSTAR interrupt-controller function
    gic_arch_extn.irq_ack = ms_eoi_irq;
    gic_arch_extn.irq_eoi = ms_eoi_irq;
    gic_arch_extn.irq_mask = ms_mask_irq;
    gic_arch_extn.irq_unmask = ms_unmask_irq;
    gic_arch_extn.irq_set_type= ms_set_type_irq;

    //Patch for disable bypass IRQ/FIQ
    {
        u32 bypass = 0;
        bypass = INREG32(GIC_PHYS + 0x2000 + GIC_CPU_CTRL);
        bypass |= GICC_DIS_BYPASS_MASK;
        OUTREG32(GIC_PHYS + 0x2000 + GIC_CPU_CTRL, bypass | GICC_ENABLE);
    }

    irq_set_chained_handler(GIC_SGI_NR+GIC_PPI_NR+INT_IRQ_PM_SLEEP, ms_handle_cascade_pm_irq);

    virq_base = irq_alloc_descs(-1, 0, PMSLEEP_FIQ_END, 0);

    infinity_irq_domain = irq_domain_add_legacy(np, PMSLEEP_FIQ_END, virq_base, hwirq_base, &ms_irq_domain_ops, NULL);
/*
    for(irq=virq_base;irq<virq_base+PMSLEEP_FIQ_END;irq++)
    {
        irq_set_chip_and_handler(irq, &ms_pm_sleep_irqchip, handle_level_irq);
        set_irq_flags(irq, IRQF_VALID | IRQF_PROBE);
    }
*/
    register_syscore_ops(&ms_irq_syscore_ops);

    return 0;
}

IRQCHIP_DECLARE(infinity, "mstar,pmsleep-intr", infinity_init_irqchip);
