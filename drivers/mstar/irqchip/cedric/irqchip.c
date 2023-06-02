#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/irqchip/arm-gic.h>
#include <linux/irqchip/chained_irq.h>
#include <linux/of_irq.h>
#include <linux/irqdomain.h>


#include <asm/irq.h>
#include <asm/setup.h>
#include <asm/mach/arch.h>
#include <asm/mach/irq.h>
#include <asm/mach/map.h>

#include <irqchip.h>

#include "cedric/irqs.h"

#include "_ms_private.h"
#include "ms_platform.h"

#define CEDRIC_INTR_AUDIO_FIRST      	1
#define CEDRIC_INTR_TO_CORE0			1

#define MS_BASE_REG_RIU_PA 		0x1F000000
#define PERI_PHYS          		0x16000000

#define INT_BASE		  		GET_REG_ADDR(MS_BASE_REG_RIU_PA,0x80C80)
#define PMU_INT_BASE	  		GET_REG_ADDR(MS_BASE_REG_RIU_PA,0x00700) //PM_SLEEP

#define CPU_ID_CORE0       		0x0
#define CPU_ID_CORE1       		0x1
//#define printk(...)


#define GIC_DIST_SET_EANBLE	    (PERI_PHYS + 0x1100)
#define CASCADE_PMU_INT 1


static DEFINE_SPINLOCK(cedric_irq_controller_lock);

static bool fiq_merged=false;


static void chip_irq_clear(unsigned int irq)
{
	unsigned short tmp;
	unsigned long int flags;
	spin_lock_irqsave(&cedric_irq_controller_lock,flags);
#if CASCADE_PMU_INT
	if(irq>127) //PMU
	{
		unsigned int pm_tmp,bit_mask;

		pm_tmp=PM_READ32(GET_REG_ADDR(PMU_INT_BASE, 0x74));
		bit_mask= (0x01 << (irq - 128));

		do
		{
			PM_WRITE32(GET_REG_ADDR(PMU_INT_BASE, 0x74),bit_mask);

		}while(0!= (bit_mask & PM_READ32(GET_REG_ADDR(PMU_INT_BASE, 0x78))));

	}
	else    //HOST
#endif
	{

		/* only FIQ can clear */
		if(irq > 47)
		{
			tmp = INREG16(GET_REG_ADDR(INT_BASE, 0x0F));
			tmp |= 0x01 << (irq - 48);
			OUTREG16(GET_REG_ADDR(INT_BASE, 0x0F),tmp);
		}
		else if(irq > 31)
		{
			tmp = INREG16(GET_REG_ADDR(INT_BASE, 0x0E));
			tmp |= 0x01 << (irq - 32);
			OUTREG16(GET_REG_ADDR(INT_BASE, 0x0E),tmp);
		}
		else if(irq > 15)
		{
			tmp = INREG16(GET_REG_ADDR(INT_BASE, 0x0D));
			tmp |= 0x01 << (irq - 16);
			OUTREG16(GET_REG_ADDR(INT_BASE, 0x0D),tmp);
		}
		else
		{
			tmp = INREG16(GET_REG_ADDR(INT_BASE, 0x0C));
			tmp |= (0x01 << irq);
			OUTREG16(GET_REG_ADDR(INT_BASE, 0x0C),tmp);
		}
	}
	spin_unlock_irqrestore(&cedric_irq_controller_lock,flags);

}

/* Clear FIQ (Clear is not supported for IRQ) */
static void ms_irq_ack(struct irq_data *data)
{

    chip_irq_clear(data->irq-INT_START);

}

static void chip_irq_mask(unsigned int irq)
{
	unsigned short tmp;
	unsigned long int flags;
	spin_lock_irqsave(&cedric_irq_controller_lock,flags);

#if CASCADE_PMU_INT
	if(irq>127) //PMU
	{

		unsigned int pm_tmp;

		pm_tmp=PM_READ32(GET_REG_ADDR(PMU_INT_BASE, 0x70));
		pm_tmp|=(0x01 << (irq - 128));

		do
		{
			PM_WRITE32(GET_REG_ADDR(PMU_INT_BASE, 0x70),pm_tmp);

		}while(pm_tmp!= PM_READ32(GET_REG_ADDR(PMU_INT_BASE, 0x70)));


	}
	else    //HOST
#endif
	{
		if(irq > 111)
		{
			tmp = INREG16(GET_REG_ADDR(INT_BASE, 0x17));
			tmp |= (0x01) << (irq - 112);
			OUTREG16(GET_REG_ADDR(INT_BASE, 0x17),tmp);
		}
		else if(irq > 95)
		{
			tmp = INREG16(GET_REG_ADDR(INT_BASE, 0x16));
			tmp |= (0x01) << (irq - 96);
			OUTREG16(GET_REG_ADDR(INT_BASE, 0x16),tmp);
		}
		else if(irq > 79)
		{
			tmp = INREG16(GET_REG_ADDR(INT_BASE, 0x15));
			tmp |= (0x01) << (irq - 80);
			OUTREG16(GET_REG_ADDR(INT_BASE, 0x15),tmp);
		}
		else if(irq > 63)
		{
			tmp = INREG16(GET_REG_ADDR(INT_BASE, 0x14));
			tmp |= (0x01) << (irq - 64);
			OUTREG16(GET_REG_ADDR(INT_BASE, 0x14),tmp);
		}
		else if(irq > 47)
		{
			tmp = INREG16(GET_REG_ADDR(INT_BASE, 0x07));
			tmp |= (0x01) << (irq - 48);
			OUTREG16(GET_REG_ADDR(INT_BASE, 0x07),tmp);
		}
		else if(irq > 31)
		{
			tmp = INREG16(GET_REG_ADDR(INT_BASE, 0x06));
			tmp |= (0x01) << (irq - 32);
			OUTREG16(GET_REG_ADDR(INT_BASE, 0x06),tmp);
		}
		else if(irq > 15)
		{
			tmp = INREG16(GET_REG_ADDR(INT_BASE, 0x05));
			tmp |= (0x01) << (irq - 16);
			OUTREG16(GET_REG_ADDR(INT_BASE, 0x05),tmp);
		}
		else
		{
			tmp = INREG16(GET_REG_ADDR(INT_BASE, 0x04));
			tmp |= (0x01)<<irq;
			OUTREG16(GET_REG_ADDR(INT_BASE, 0x04),tmp);
		}

	}
	spin_unlock_irqrestore(&cedric_irq_controller_lock,flags);

}

/* Mask IRQ/FIQ */
static void ms_irq_mask(struct irq_data *data)
{

    chip_irq_mask(data->irq-INT_START);
}

static void chip_irq_unmask(unsigned int irq)
{
	unsigned short tmp;
	unsigned long int flags;
	spin_lock_irqsave(&cedric_irq_controller_lock,flags);

#if CASCADE_PMU_INT
	if(irq>127) //PMU
	{

		unsigned int pm_tmp;

		pm_tmp=PM_READ32(GET_REG_ADDR(PMU_INT_BASE, 0x70));
		pm_tmp &= ~(0x01 << (irq - 128));

		do
		{
			PM_WRITE32(GET_REG_ADDR(PMU_INT_BASE, 0x70),pm_tmp);

		}while(pm_tmp!= PM_READ32(GET_REG_ADDR(PMU_INT_BASE, 0x70)));


	}
	else    //HOST
#endif
	{
		if(irq > 111)
		{
			tmp = INREG16(GET_REG_ADDR(INT_BASE, 0x17));
			tmp &= ~( 0x01 << (irq - 112));
			OUTREG16(GET_REG_ADDR(INT_BASE, 0x17),tmp);
		}
		else if(irq > 95)
		{
			tmp = INREG16(GET_REG_ADDR(INT_BASE, 0x16));
			tmp &= ~( 0x01 << (irq - 96));
			OUTREG16(GET_REG_ADDR(INT_BASE, 0x16),tmp);
		}
		else if(irq > 79)
		{
			tmp = INREG16(GET_REG_ADDR(INT_BASE, 0x15));
			tmp &= ~( 0x01 << (irq - 80));
			OUTREG16(GET_REG_ADDR(INT_BASE, 0x15),tmp);
		}
		else if(irq > 63)
		{
			tmp = INREG16(GET_REG_ADDR(INT_BASE, 0x14));
			tmp &= ~( 0x01 << (irq - 64));
			OUTREG16(GET_REG_ADDR(INT_BASE, 0x14),tmp);
		}
		else if(irq > 47)
		{
			tmp = INREG16(GET_REG_ADDR(INT_BASE, 0x07));
			tmp &= ~( 0x01 << (irq - 48));
			OUTREG16(GET_REG_ADDR(INT_BASE, 0x07),tmp);
		}
		else if(irq > 31)
		{
			tmp = INREG16(GET_REG_ADDR(INT_BASE, 0x06));
			tmp &= ~( 0x01 << (irq - 32));
			OUTREG16(GET_REG_ADDR(INT_BASE, 0x06),tmp);
		}
		else if(irq > 15)
		{
			tmp = INREG16(GET_REG_ADDR(INT_BASE, 0x05));
			tmp &= ~( 0x01 << (irq - 16));
			OUTREG16(GET_REG_ADDR(INT_BASE, 0x05),tmp);
		}
		else
		{
			tmp = INREG16(GET_REG_ADDR(INT_BASE, 0x04));
			tmp &= ~( 0x01 << irq);
			OUTREG16(GET_REG_ADDR(INT_BASE, 0x04),tmp);
		}

	}
	spin_unlock_irqrestore(&cedric_irq_controller_lock,flags);

}
/* Un-Mask IRQ/FIQ */
static void ms_irq_unmask(struct irq_data *data) {

    chip_irq_unmask(data->irq-INT_START);

}

static void ms_irq_disable(struct irq_data *data) {

    ms_irq_mask(data);
}

//static unsigned char chip_irq_get_polarity(unsigned int irq)
//{
//	unsigned short tmp;
//
//#if CASCADE_PMU_INT
//	if(irq>127) //PMU
//	{
//
//		unsigned int pm_tmp;
//
//		pm_tmp=PM_READ32(GET_REG_ADDR(PMU_INT_BASE, 0x7a));
//		pm_tmp &= (0x01 << (irq - 128));
//
//		return ( pm_tmp > 0  )? 1:0;
//
//	}
//	else    //HOST
//#endif
//	{
//		if(irq > 111)
//		{
//			tmp = INREG16(GET_REG_ADDR(INT_BASE, 0x1b));
//			return ((tmp & ( 0x01 << (irq - 112) )) > 0  )? 1:0;
//		}
//		else if(irq > 95)
//		{
//			tmp = INREG16(GET_REG_ADDR(INT_BASE, 0x1a));
//			return ((tmp & ( 0x01 << (irq - 96) )) > 0  )? 1:0;
//		}
//		else if(irq > 79)
//		{
//			tmp = INREG16(GET_REG_ADDR(INT_BASE, 0x19));
//			return ((tmp & ( 0x01 << (irq - 80) )) > 0  )? 1:0;
//		}
//		else if(irq > 63)
//		{
//			tmp = INREG16(GET_REG_ADDR(INT_BASE, 0x18));
//			return ((tmp & ( 0x01 << (irq - 64) )) > 0  )? 1:0;
//		}
//		else if(irq > 47)
//		{
//			tmp = INREG16(GET_REG_ADDR(INT_BASE, 0x0b));
//			return ((tmp & ( 0x01 << (irq - 48) )) > 0  )? 1:0;
//		}
//		else if(irq > 31)
//		{
//			tmp = INREG16(GET_REG_ADDR(INT_BASE, 0x0a));
//			return ((tmp & ( 0x01 << (irq - 32) )) > 0  )? 1:0;
//		}
//		else if(irq > 15)
//		{
//			tmp = INREG16(GET_REG_ADDR(INT_BASE, 0x09));
//			return ((tmp & ( 0x01 << (irq - 16) )) > 0  )? 1:0;
//		}
//		else
//		{
//			tmp = INREG16(GET_REG_ADDR(INT_BASE, 0x08));
//			return ((tmp & ( 0x01 << irq )) > 0  )? 1:0;
//		}
//
//	}
//}


static void chip_irq_set_polarity(unsigned int irq,unsigned char polarity)
{
	unsigned long int flags;
	spin_lock_irqsave(&cedric_irq_controller_lock,flags);
	if(polarity>0) //reverse
	{
#if CASCADE_PMU_INT
		if(irq>127) //PMU
		{
			unsigned int pm_tmp;

			pm_tmp=PM_READ32(GET_REG_ADDR(PMU_INT_BASE, 0x7a));
			pm_tmp |= (0x01 << (irq - 128));

			do
			{
				PM_WRITE32(GET_REG_ADDR(PMU_INT_BASE, 0x7a),pm_tmp);

			}while(pm_tmp!= PM_READ32(GET_REG_ADDR(PMU_INT_BASE, 0x7a)));



		}
		else    //HOST
#endif
		{
			if(irq > 111)
			{
				SETREG16(GET_REG_ADDR(INT_BASE, 0x1b),( 0x01 << (irq - 112) ));
			}
			else if(irq > 95)
			{
				SETREG16(GET_REG_ADDR(INT_BASE, 0x1a),( 0x01 << (irq - 96) ));
			}
			else if(irq > 79)
			{
				SETREG16(GET_REG_ADDR(INT_BASE, 0x19),( 0x01 << (irq - 80) ));
			}
			else if(irq > 63)
			{
				SETREG16(GET_REG_ADDR(INT_BASE, 0x18),( 0x01 << (irq - 64) ));
			}
			else if(irq > 47)
			{
				SETREG16(GET_REG_ADDR(INT_BASE, 0x0b),( 0x01 << (irq - 47) ));
			}
			else if(irq > 31)
			{
				SETREG16(GET_REG_ADDR(INT_BASE, 0x0a),( 0x01 << (irq - 32) ));
			}
			else if(irq > 15)
			{
				SETREG16(GET_REG_ADDR(INT_BASE, 0x09),( 0x01 << (irq - 16) ));
			}
			else
			{
				SETREG16(GET_REG_ADDR(INT_BASE, 0x08),( 0x01 << irq ));
			}

		}
	}
	else
	{

#if CASCADE_PMU_INT
		if(irq>127) //PMU
		{

			unsigned int pm_tmp;

			pm_tmp=PM_READ32(GET_REG_ADDR(PMU_INT_BASE, 0x7a));
			pm_tmp &= ~(0x01 << (irq - 128));

			do
			{
				PM_WRITE32(GET_REG_ADDR(PMU_INT_BASE, 0x7a),pm_tmp);

			}while(pm_tmp!= PM_READ32(GET_REG_ADDR(PMU_INT_BASE, 0x7a)));
		}
		else    //HOST
#endif
		{
			if(irq > 111)
			{
				CLRREG16(GET_REG_ADDR(INT_BASE, 0x1b),( 0x01 << (irq - 112) ));
			}
			else if(irq > 95)
			{
				CLRREG16(GET_REG_ADDR(INT_BASE, 0x1a),( 0x01 << (irq - 96) ));
			}
			else if(irq > 79)
			{
				CLRREG16(GET_REG_ADDR(INT_BASE, 0x19),( 0x01 << (irq - 80) ));
			}
			else if(irq > 63)
			{
				CLRREG16(GET_REG_ADDR(INT_BASE, 0x18),( 0x01 << (irq - 64) ));
			}
			else if(irq > 47)
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
	spin_unlock_irqrestore(&cedric_irq_controller_lock,flags);
}

static int ms_irq_set_affinity(struct irq_data *data, const struct cpumask *dest, bool force)
{
	//use a very simple implementation here...
	return irq_get_chip(GIC_ID_LEGACY_IRQ)->irq_set_affinity(irq_get_irq_data(GIC_ID_LEGACY_IRQ),dest,force);
}


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
//static unsigned char ms_irq_get_polarity(struct irq_data *data)
//{
//
//    return chip_irq_get_polarity(data->irq-INT_START);
//}
//
//
//
//static void ms_irq_set_polarity(struct irq_data *data,unsigned char polarity)
//{
//
//    chip_irq_set_polarity(data->irq-INT_START,polarity);
//}


static unsigned int __find_int_nr(unsigned base, unsigned int status_reg_offset, unsigned int bank_nr, unsigned int intr_start, unsigned int  intr_max)
{
	unsigned int i=0, status=0, intr=0, intr_idx=0;

	intr=intr_start;

	for(i=0;i<bank_nr;i++)
	{
		status=INREG16(GET_REG_ADDR(base,(status_reg_offset+i)));
		if(0!=status)
		{
			break;

		}
		intr+=16;
	}

	if(intr_max==intr)
	{
		return 0xFFFFFFFF;
	}

	for(i=0;i<16;i++)
	{
		if(0!=(status & (1<<i)))
		{

			break;
		}

		intr_idx+=1;
	}

	if(16==intr_idx)
	{
		return 0xFFFFFFFF;
	}

	return (intr+intr_idx);


}


static unsigned int __find_pm_int_nr(unsigned int intr_start)
{
	unsigned int i=0, status=0, intr_idx=0;

	status=PM_READ32(GET_REG_ADDR(PMU_INT_BASE,0x78));

	// status length must be 32
	for(i=0;i<32;i++)
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

#if CASCADE_PMU_INT
static void ms_irq_handle_cascade_pmu_irq(unsigned int irq, struct irq_desc *desc)
{
	unsigned int cascade_irq=0xFFFFFFFF;
	struct irq_chip *chip = irq_get_chip(irq);


	chained_irq_enter(chip, desc);

	spin_lock(&cedric_irq_controller_lock);
	{

		cascade_irq=__find_pm_int_nr(PMU_START);

	}
	spin_unlock(&cedric_irq_controller_lock);

	if(0xFFFFFFFF==cascade_irq)
		BUG();

	generic_handle_irq(cascade_irq);


	chained_irq_exit(chip, desc);

}
#endif





void __iomem *_gic_cpu_base_addr =(void __iomem *)(IO_ADDRESS(PERI_PHYS + 0x0100));
void __iomem *_gic_dist_base_addr=(void __iomem *)(IO_ADDRESS(PERI_PHYS + 0x1000));


static void ms_irq_handle_cascade_host0_irq(unsigned int irq, struct irq_desc *desc)
{
	unsigned int cascade_irq=0xFFFFFFFF;
	struct irq_chip *chip = irq_get_chip(irq);
	unsigned long flag=0;
    static unsigned int err=0;

    //printk("[ms_irq_handle_cascade_host0_irq] enter");
	chained_irq_enter(chip, desc);

	spin_lock_irqsave(&cedric_irq_controller_lock,flag);

	{

#if (CEDRIC_INTR_AUDIO_FIRST)
		// for AUDIO fast path
		if( INREG16(GET_REG_ADDR(INT_BASE,0x1E)) & (1<<((INT_IRQ_AU_SYSTEM-IRQ_START)%16)) )
		{
			cascade_irq=INT_IRQ_AU_SYSTEM;
		}
		else
#endif
		{

			if(fiq_merged)
			{
				cascade_irq=__find_int_nr(INT_BASE,0x0C,4,FIQ_START,FIQ_END);
			}

			if(0xFFFFFFFF==cascade_irq)
			{
				cascade_irq=__find_int_nr(INT_BASE,0x1C,4,IRQ_START,IRQ_END);
			}
		}

	}
	spin_unlock_irqrestore(&cedric_irq_controller_lock, flag);

	if(0xFFFFFFFF!=cascade_irq)
	{
		generic_handle_irq(cascade_irq);
	}
	else
	{
        err++;
        if(err >= 1000)
        {
            printk(KERN_ERR "[irqchip-cedric] LEVEL0 IRQ raised but can not be found!!\n");
            //BUG();
            err = 0;
        }
	}

	chained_irq_exit(chip, desc);

}




static struct irq_chip ms_irq_chip = {
	.name = "Cedric IRQCHIP",
	.irq_ack = ms_irq_ack,
	.irq_mask = ms_irq_mask,
	.irq_unmask = ms_irq_unmask,
	.irq_disable = ms_irq_disable,
	.irq_set_type = ms_irq_set_type,
	.irq_set_affinity=ms_irq_set_affinity,
};


static struct irq_domain *cedric_irq_domain;

static struct irq_domain_ops ms_irq_domain_ops = {
//	.map = ms_irq_domain_map,
	.xlate = irq_domain_xlate_onecell,
};



static int __init ms_irq_of_init(struct device_node *np, struct device_node *interrupt_parent)
{

	int  irq_base=0;
	int irq=0;
	int temp=0;


	//mask all host0 IRQ
	OUTREG16(GET_REG_ADDR(INT_BASE, 0x17),0xFFFF);
	OUTREG16(GET_REG_ADDR(INT_BASE, 0x16),0xFFFF);
	OUTREG16(GET_REG_ADDR(INT_BASE, 0x15),0xFFFF);
	OUTREG16(GET_REG_ADDR(INT_BASE, 0x14),0xFFFF);
	OUTREG16(GET_REG_ADDR(INT_BASE, 0x07),0xFFFF);
	OUTREG16(GET_REG_ADDR(INT_BASE, 0x06),0xFFFF);
	OUTREG16(GET_REG_ADDR(INT_BASE, 0x05),0xFFFF);
	OUTREG16(GET_REG_ADDR(INT_BASE, 0x04),0xFFFF);

	SETREG16(GET_REG_ADDR(GET_REG_ADDR(MS_BASE_REG_RIU_PA, 0x080E80),0x37), 0x20);
	fiq_merged=true;




	irq_base = irq_alloc_descs(-1, INT_START, ((PMU_END-INT_START)+1), 0);
	BUG_ON(irq_base < 0);

	cedric_irq_domain = irq_domain_add_legacy(np, ((PMU_END-INT_START)+1), irq_base, INT_START, &ms_irq_domain_ops, NULL);



	{
		gic_init_bases(0,29,_gic_dist_base_addr,_gic_cpu_base_addr,0,NULL);


		temp=INREG32(GIC_DIST_SET_EANBLE+((GIC_ID_LEGACY_IRQ/32)*4));
		temp= temp | (0x1 << (GIC_ID_LEGACY_IRQ % 32));
		OUTREG32(GIC_DIST_SET_EANBLE+((GIC_ID_LEGACY_IRQ/32)*4),temp);

#if defined(CONFIG_HAVE_ARM_TWD)
		temp=INREG32(GIC_DIST_SET_EANBLE+((GIC_ID_LOCAL_TIMER_IRQ/32)*4));
		temp= temp | (0x1 << (GIC_ID_LOCAL_TIMER_IRQ % 32));
		OUTREG32(GIC_DIST_SET_EANBLE+((GIC_ID_LOCAL_TIMER_IRQ/32)*4),temp);
#endif


		irq_set_chained_handler(GIC_ID_LEGACY_IRQ, ms_irq_handle_cascade_host0_irq);

		#if defined(CEDRIC_INTR_TO_CORE0)
				irq_set_affinity(GIC_ID_LEGACY_IRQ,cpumask_of(CPU_ID_CORE0));
		#endif



		for(irq=FIQ_START;irq<IRQ_END;irq++)
		{
			irq_set_chip_and_handler(irq, &ms_irq_chip, handle_level_irq);
			set_irq_flags(irq, IRQF_VALID | IRQF_PROBE);
#if defined(CEDRIC_INTR_TO_CORE0)
			irq_set_affinity(irq,cpumask_of(CPU_ID_CORE0));
#endif

		}



#if CASCADE_PMU_INT

		chip_irq_set_polarity(INT_PMU_SD_DETECT0-INT_START,1);
		chip_irq_set_polarity(INT_PMU_SD_DETECT1-INT_START,1);
		chip_irq_set_polarity(INT_PMU_SD_DETECT2-INT_START,1);
		chip_irq_set_polarity(INT_PMU_SD_DETECT3-INT_START,1);

		chip_irq_clear(INT_PMU_SD_DETECT0-INT_START);
		chip_irq_clear(INT_PMU_SD_DETECT1-INT_START);
		chip_irq_clear(INT_PMU_SD_DETECT2-INT_START);
		chip_irq_clear(INT_PMU_SD_DETECT3-INT_START);

		irq_set_chained_handler(INT_IRQ_PMU, ms_irq_handle_cascade_pmu_irq);
		//set irq handler
		for(irq=PMU_START;irq<PMU_END;irq++)
		{
			irq_set_chip_and_handler(irq, &ms_irq_chip, handle_level_irq);
			set_irq_flags(irq, IRQF_VALID | IRQF_PROBE);

#if defined(CEDRIC_INTR_TO_CORE0)
			irq_set_affinity(irq,cpumask_of(CPU_ID_CORE0));
#endif

		}

#endif

	}

	return 0;

}



IRQCHIP_DECLARE(cedric, "mstar,intrctl-cedric", ms_irq_of_init);
