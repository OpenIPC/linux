
#include <linux/init.h>
//#include <linux/irqchip/arm-gic.h>
#include <linux/irqchip/chained_irq.h>
#include <asm/mach/irq.h>
#include <linux/interrupt.h>

#include <linux/of_irq.h>
#include <irqchip.h>

#include "chicago/irqs.h"
#include "chicago/registers.h"

#include "_ms_private.h"
#include "ms_platform.h"


#define MS_BASE_REG_RIU_PA 		0x1F000000
#define PERI_PHYS          		0x16000000

#define INT_BASE		  GET_REG_ADDR(MS_BASE_REG_RIU_PA,0x001580)
#define PMU_INT_BASE	  GET_REG_ADDR(MS_BASE_REG_RIU_PA,0x001F80) //PMU

//#define printk(...)




static DEFINE_SPINLOCK(irq_controller_lock);


static void chip_irq_clear(unsigned int irq)
{
    unsigned short tmp;

    if(irq>63) //PMU
    {
        unsigned int pm_tmp,bit_mask;
        //printk(KERN_INFO"%s %d\n",__func__ , irq);
        pm_tmp=PM_READ32(GET_REG_ADDR(PMU_INT_BASE, 0x74));
        bit_mask= (0x01 << (irq - 64));

        do
        {
            PM_WRITE32(GET_REG_ADDR(PMU_INT_BASE, 0x74),bit_mask);

        }
        while(0!= (bit_mask & PM_READ32(GET_REG_ADDR(PMU_INT_BASE, 0x78))));
    }
    else    //HOST
    {

        /* only FIQ can clear */
        if(irq > 15)
        {
            tmp = INREG16(GET_REG_ADDR(INT_BASE, 0x1D));
            tmp |= 0x01 << (irq - 16);
            OUTREG16(GET_REG_ADDR(INT_BASE, 0x05),tmp);
        }
        else
        {
            tmp = INREG16(GET_REG_ADDR(INT_BASE, 0x1C));
            tmp |= (0x01 << irq);
            OUTREG16(GET_REG_ADDR(INT_BASE, 0x04),tmp);
        }
    }


}

/* Clear FIQ (Clear is not supported for IRQ) */
static void ms_irq_ack(struct irq_data *data)
{

    chip_irq_clear(data->irq-INT_START);

}

static void chip_irq_mask(unsigned int irq)
{
    unsigned short tmp;

    if(irq>63) //PMU
    {

        unsigned int pm_tmp;
        //printk(KERN_INFO"%s %d\n",__func__ , irq);
        pm_tmp=PM_READ32(GET_REG_ADDR(PMU_INT_BASE, 0x70));
        pm_tmp|=(0x01 << (irq - 64));

        do
        {
            PM_WRITE32(GET_REG_ADDR(PMU_INT_BASE, 0x70),pm_tmp);

        }
        while(pm_tmp!= PM_READ32(GET_REG_ADDR(PMU_INT_BASE, 0x70)));


    }
    else    //HOST

    {
        if(irq > 47)
        {
            tmp = INREG16(GET_REG_ADDR(INT_BASE, 0x0D));
            tmp |= (0x01) << (irq - 48);
            OUTREG16(GET_REG_ADDR(INT_BASE, 0x0D),tmp);
        }
        else if(irq > 31)
        {
            tmp = INREG16(GET_REG_ADDR(INT_BASE, 0x0C));
            tmp |= (0x01) << (irq - 32);
            OUTREG16(GET_REG_ADDR(INT_BASE, 0x0C),tmp);
        }
        else if(irq > 15)
        {
            tmp = INREG16(GET_REG_ADDR(INT_BASE, 0x01));
            tmp |= (0x01) << (irq - 16);
            OUTREG16(GET_REG_ADDR(INT_BASE, 0x01),tmp);
        }
        else
        {
            tmp = INREG16(GET_REG_ADDR(INT_BASE, 0x00));
            tmp |= (0x01)<<irq;
            OUTREG16(GET_REG_ADDR(INT_BASE, 0x00),tmp);
        }

    }
}

/* Mask IRQ/FIQ */
static void ms_irq_mask(struct irq_data *data)
{

    chip_irq_mask(data->irq-INT_START);
}

static void chip_irq_unmask(unsigned int irq)
{
    unsigned short tmp;

    if(irq>63) //PMU
    {

        unsigned int pm_tmp;
        //printk(KERN_INFO"%s %d\n",__func__ , irq);
        pm_tmp=PM_READ32(GET_REG_ADDR(PMU_INT_BASE, 0x70));
        pm_tmp &= ~(0x01 << (irq - 64));

        do
        {
            PM_WRITE32(GET_REG_ADDR(PMU_INT_BASE, 0x70),pm_tmp);

        }
        while(pm_tmp!= PM_READ32(GET_REG_ADDR(PMU_INT_BASE, 0x70)));


    }
    else    //HOST

    {

        if(irq > 47)
        {
            tmp = INREG16(GET_REG_ADDR(INT_BASE, 0x0D));
            tmp &= ~( 0x01 << (irq - 48));
            OUTREG16(GET_REG_ADDR(INT_BASE, 0x0D),tmp);
        }
        else if(irq > 31)
        {
            tmp = INREG16(GET_REG_ADDR(INT_BASE, 0x0C));
            tmp &= ~( 0x01 << (irq - 32));
            OUTREG16(GET_REG_ADDR(INT_BASE, 0x0C),tmp);
        }
        else if(irq > 15)
        {
            tmp = INREG16(GET_REG_ADDR(INT_BASE, 0x01));
            tmp &= ~( 0x01 << (irq - 16));
            OUTREG16(GET_REG_ADDR(INT_BASE, 0x01),tmp);
        }
        else
        {
            tmp = INREG16(GET_REG_ADDR(INT_BASE, 0x00));
            tmp &= ~( 0x01 << irq);
            OUTREG16(GET_REG_ADDR(INT_BASE, 0x00),tmp);
        }

    }
}
/* Un-Mask IRQ/FIQ */
static void ms_irq_unmask(struct irq_data *data)
{


    //printk(KERN_INFO"UN Mask IRQ/FIQ:%d\n", (data->irq-INT_START));

    chip_irq_unmask(data->irq-INT_START);

}

static void ms_irq_disable(struct irq_data *data)
{

    ms_irq_mask(data);
}


#if 0
unsigned char chip_irq_get_polarity(unsigned int irq)
{
    unsigned short tmp;

    if(irq>63) //PMU
    {

        unsigned int pm_tmp;

        pm_tmp=PM_READ32(GET_REG_ADDR(PMU_INT_BASE, 0x7a));
        pm_tmp &= (0x01 << (irq - 64));

        return ( pm_tmp > 0  )? 1:0;

    }
    else    //HOST
    {

        if(irq > 47)
        {
            tmp = INREG16(GET_REG_ADDR(INT_BASE, 0x15));
            return ((tmp & ( 0x01 << (irq - 48) )) > 0  )? 1:0;
        }
        else if(irq > 31)
        {
            tmp = INREG16(GET_REG_ADDR(INT_BASE, 0x14));
            return ((tmp & ( 0x01 << (irq - 32) )) > 0  )? 1:0;
        }
        else if(irq > 15)
        {
            tmp = INREG16(GET_REG_ADDR(INT_BASE, 0x0B));
            return ((tmp & ( 0x01 << (irq - 16) )) > 0  )? 1:0;
        }
        else
        {
            tmp = INREG16(GET_REG_ADDR(INT_BASE, 0x0A));
            return ((tmp & ( 0x01 << irq )) > 0  )? 1:0;
        }

    }
}
#endif

static void chip_irq_set_polarity(unsigned int irq,unsigned char polarity)
{

    if(polarity>0)
    {
        if(irq>63) //PMU
        {
            unsigned int pm_tmp;

            pm_tmp=PM_READ32(GET_REG_ADDR(PMU_INT_BASE, 0x7a));
            pm_tmp |= (0x01 << (irq - 64));

            do
            {
                PM_WRITE32(GET_REG_ADDR(PMU_INT_BASE, 0x7a),pm_tmp);

            }
            while(pm_tmp!= PM_READ32(GET_REG_ADDR(PMU_INT_BASE, 0x7a)));



        }
        else    //HOST
        {
            if(irq > 47)
            {
                SETREG16(GET_REG_ADDR(INT_BASE, 0x15),( 0x01 << (irq - 47) ));
            }
            else if(irq > 31)
            {
                SETREG16(GET_REG_ADDR(INT_BASE, 0x14),( 0x01 << (irq - 32) ));
            }
            else if(irq > 15)
            {
                SETREG16(GET_REG_ADDR(INT_BASE, 0x0B),( 0x01 << (irq - 16) ));
            }
            else
            {
                SETREG16(GET_REG_ADDR(INT_BASE, 0x0A),( 0x01 << irq ));
            }

        }
    }
    else
    {

        if(irq>63) //PMU
        {

            unsigned int pm_tmp;

            pm_tmp=PM_READ32(GET_REG_ADDR(PMU_INT_BASE, 0x7a));
            pm_tmp &= ~(0x01 << (irq - 64));

            do
            {
                PM_WRITE32(GET_REG_ADDR(PMU_INT_BASE, 0x7a),pm_tmp);

            }
            while(pm_tmp!= PM_READ32(GET_REG_ADDR(PMU_INT_BASE, 0x7a)));
        }
        else    //HOST
        {
            if(irq > 47)
            {
                CLRREG16(GET_REG_ADDR(INT_BASE, 0x0b),( 0x01 << (irq - 47) ));
            }
            else if(irq > 31)
            {
                CLRREG16(GET_REG_ADDR(INT_BASE, 0x0a),( 0x01 << (irq - 32) ));
            }
            else if(irq > 15)
            {
                CLRREG16(GET_REG_ADDR(INT_BASE, 0x09),( 0x01 << (irq - 16) ));
            }
            else
            {
                CLRREG16(GET_REG_ADDR(INT_BASE, 0x08),( 0x01 << irq ));
            }

        }

    }
}

//#if defined(CONFIG_SMP)
//static int ms_irq_set_affinity(struct irq_data *data, const struct cpumask *dest, bool force)
//{
//	//use a very simple implementation here...
//	return irq_get_chip(GIC_ID_LEGACY_IRQ)->irq_set_affinity(irq_get_irq_data(GIC_ID_LEGACY_IRQ),dest,force);
//
//
//}
//#endif




static int	ms_irq_set_type(struct irq_data *data, unsigned int flow_type)
{

    if(flow_type & (IRQ_TYPE_EDGE_FALLING|IRQ_TYPE_LEVEL_HIGH))
    {
        chip_irq_set_polarity(data->irq,0);
    }
    else
    {
        chip_irq_set_polarity(data->irq,1);
    }

    return 0;
}

#if 0
static unsigned char ms_irq_get_polarity(struct irq_data *data)
{

    return chip_irq_get_polarity(data->irq-INT_START);
}



static void ms_irq_set_polarity(struct irq_data *data,unsigned char polarity)
{

    chip_irq_set_polarity(data->irq-INT_START,polarity);

}
#endif


static unsigned int __find_pm_int_nr(unsigned int intr_start)
{
    unsigned int i=0, status=0, intr_idx=0;

    status=PM_READ32(GET_REG_ADDR(PMU_INT_BASE,0x78));

    // status length must be 32
    for(i=0; i<32; i++)
    {
        if(0!=(status & (1<<i)))
        {

            break;
        }

        intr_idx+=1;
    }

    // status length must be 32
    if(32==intr_idx)
    {
        return 0xFFFFFFFF;
    }

    return (intr_start+intr_idx);

}

static void ms_irq_handle_cascade_pmu_irq(unsigned int irq, struct irq_desc *desc)
{
    unsigned int cascade_irq=0xFFFFFFFF;
    struct irq_chip *chip = irq_get_chip(irq);


    chained_irq_enter(chip, desc);

    spin_lock(&irq_controller_lock);
    {

        cascade_irq=__find_pm_int_nr(PMU_START);

    }
    spin_unlock(&irq_controller_lock);

    if(0xFFFFFFFF==cascade_irq)
        BUG();

    printk(KERN_INFO"cascade_pmu_irq %d cascade:%d\n", irq, cascade_irq);

    generic_handle_irq(cascade_irq);


    chained_irq_exit(chip, desc);

}





static struct irq_chip ms_irq_chip =
{
    .name = "Chicago IRQCHIP",
    .irq_ack = ms_irq_ack,
    .irq_mask = ms_irq_mask,
    .irq_unmask = ms_irq_unmask,
    .irq_disable = ms_irq_disable,
  	.irq_set_type = ms_irq_set_type,
//#if defined(CONFIG_SMP)
//	.irq_set_affinity=ms_irq_set_affinity,
//#endif
};

static struct irq_domain *chicago_irq_domain;

static struct irq_domain_ops chicago_irq_domain_ops = {
	//.map = ms_irq_domain_map,
	.xlate = irq_domain_xlate_onecell,
};


static void ms_irq_handle_irq(struct pt_regs *regs)
{
	unsigned int u32Irq=0xFFFFFFFF;
	//struct irq_chip *chip = irq_get_chip(irq);
	//unsigned long flag=0;
    //static unsigned int err=0;
    unsigned char u8IrqStatus = 0;
    unsigned char u8FiqStatus = 0;

   	
    // Get pending interrupt(s)
    u8FiqStatus = INREG8(GET_REG_ADDR(INT_BASE, 0x22));
    u8IrqStatus = INREG8(GET_REG_ADDR(INT_BASE, 0x23));

    // Check which FIQ/IRQ interrupt occurs
    u32Irq = (u8FiqStatus != 0xFF) ? u8FiqStatus : u8IrqStatus;

    
    handle_domain_irq(chicago_irq_domain, u32Irq, regs);
}


static void ms_ctl_init(void)
{
    //mask all host0 IRQ
    OUTREG16(GET_REG_ADDR(INT_BASE, 0x0D),0xFFFF);
    OUTREG16(GET_REG_ADDR(INT_BASE, 0x0C),0xFFFF);
    OUTREG16(GET_REG_ADDR(INT_BASE, 0x01),0xFFFF);
    OUTREG16(GET_REG_ADDR(INT_BASE, 0x00),0xFFFF);

    //clear all host0 interrupt
    OUTREG16(GET_REG_ADDR(INT_BASE, 0x04),0xFFFF);
    OUTREG16(GET_REG_ADDR(INT_BASE, 0x05),0xFFFF);

    //Mask all pmu interrupt
    do
    {
        OUTREG16(GET_REG_ADDR(PMU_INT_BASE, 0x70), 0xFFFF);
        OUTREG16(GET_REG_ADDR(PMU_INT_BASE, 0x71), 0xFFFF);
    }
    while(INREG16(GET_REG_ADDR(PMU_INT_BASE, 0x70)) != 0xFFFF && INREG16(GET_REG_ADDR(PMU_INT_BASE, 0x71)) != 0xFFFF);

    //Clear all pmu interrupt
    OUTREG16(GET_REG_ADDR(PMU_INT_BASE, 0x74), 0xFFFF);
    OUTREG16(GET_REG_ADDR(PMU_INT_BASE, 0x75), 0xFFFF);
}


static int __init ms_irq_of_init(struct device_node *np, struct device_node *interrupt_parent)
{
    int irq=0;
    int irq_base=0;

    ms_ctl_init();

    for(irq=FIQ_START; irq<IRQ_END; irq++)
    {
		irq_set_chip(irq, &ms_irq_chip);
    	irq_set_handler(irq, handle_level_irq);
        set_irq_flags(irq, IRQF_VALID | IRQF_PROBE);
    }

    irq_set_chained_handler(INT_IRQ_PMU, ms_irq_handle_cascade_pmu_irq);
    //set irq handler
    for(irq=PMU_START; irq<PMU_END; irq++)
    {
        irq_set_chip_and_handler(irq, &ms_irq_chip, handle_level_irq);
        set_irq_flags(irq, IRQF_VALID | IRQF_PROBE);
    }

	irq_base = irq_alloc_descs(-1, INT_START, CHICAGO_NR_IRQS, 0);
	BUG_ON(irq_base < 0);

	chicago_irq_domain = irq_domain_add_legacy(np, CHICAGO_NR_IRQS, INT_START, INT_START, &chicago_irq_domain_ops, NULL);
    if (!chicago_irq_domain) {
        printk( KERN_INFO "Failed to add core IRQ domain\n");
    	return -EINVAL;
	}

    set_handle_irq(ms_irq_handle_irq);
    return 0;
}


IRQCHIP_DECLARE(chicago, "mstar,intrctl-chicago", ms_irq_of_init);


