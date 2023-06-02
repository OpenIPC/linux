
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/irqchip.h>
#include <linux/of_platform.h>
#include <linux/sys_soc.h>
#include <linux/slab.h>


#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/memory.h>
#include <asm/io.h>
#include <asm/mach/map.h>

#include "chicago/registers.h"
#include "chicago/irqs.h"
#include "ms_platform.h"
#include "ms_types.h"
#include "_ms_private.h"

#define CHICAGO_MIU0_BASE      0x20000000

#define IO_PHYS                 0x1F000000
#define IO_VIRT                 (IO_PHYS+IO_OFFSET)//from IO_ADDRESS(x)
#define IO_OFFSET               (MS_IO_OFFSET)
#define IO_SIZE                 0x00400000


//#define IMI_PHYS                0xA0000000
//#define IMI_VIRT                0xF9000000
//#define INI_OFFSET              (IMI_VIRT-IMI_PHYS)
//#define IMI_SIZE                0x50000


extern struct ms_chip* ms_chip_get(void);
extern void __init ms_chip_init_default(void);
extern void chicago_reset_cpu(int i);


#define PLATFORM_NAME PLATFORM_NAME_CHICAGO

static void chicago_restart(enum reboot_mode mode, const char *cmd)
{        
	chicago_reset_cpu(0);
}


/* IO tables */
static struct map_desc chicago_io_desc[] __initdata =
{
    /* Define Registers' physcial and virtual addresses */
    {
        .virtual        = IO_VIRT,
        .pfn            = __phys_to_pfn(IO_PHYS),
        .length         = IO_SIZE,
        .type           = MT_DEVICE
    },

/*
    {
        .virtual        = PERI_VIRT,
        .pfn            = __phys_to_pfn(PERI_PHYS),
        .length         = PERI_SIZE,
        .type           = MT_DEVICE
    },
*/

/*
    {
        .virtual        = IMI_VIRT,
        .pfn            = __phys_to_pfn(IMI_PHYS),
        .length         = IMI_SIZE,
        .type           = MT_DEVICE,
    },
*/
};


static const char *chicago_dt_compat[] __initconst = {
    "mstar,chicago",
    NULL,
};

static void __init chicago_map_io(void)
{
    iotable_init(chicago_io_desc, ARRAY_SIZE(chicago_io_desc));

}

/*************************************
*        Mstar chip flush function
*************************************/

static DEFINE_SPINLOCK(chicago_l2prefetch_lock);

static void chicago_uart_disable_line(int line)
{
    if(line == 0) 
    {
        //CLRREG16(BASE_REG_PADTOP_PA + REG_ID_03, BIT0);
    }
    else if(line == 1)
    {
        CLRREG16(BASE_REG_PADTOP_PA + REG_ID_03, BIT1);
    }
    else if(line==2)
    {
        CLRREG16(BASE_REG_PADTOP_PA + REG_ID_03, BIT3);
    }
    else if(line==3)
    {
        CLRREG16(BASE_REG_PADTOP_PA + REG_ID_03, BIT5);
    }
}

static void chicago_uart_enable_line(int line)
{
    if(line == 0)
    {
        //SETREG16(BASE_REG_PADTOP_PA + REG_ID_03, BIT0);
    }
    else if(line == 1)
    {
        SETREG16(BASE_REG_PADTOP_PA + REG_ID_03, BIT1);
    }
    else if(line==2)
    {
        SETREG16(BASE_REG_PADTOP_PA + REG_ID_03, BIT3);
    }
    else if(line==3)
    {
        SETREG16(BASE_REG_PADTOP_PA + REG_ID_03, BIT5);
    }
}

static int chicago_get_device_id(void)
{
    return (int)DEVICE_ID_CHICAGO;
}

static int chicago_get_revision(void)
{
    u16 tmp = 0;
        tmp = INREG16((unsigned int)(BASE_REG_CHIPTOP_PA + REG_ID_67));
    tmp=((tmp >> 8) & 0x00FF);

    return (tmp+1);
}


static void chicago_chip_flush_miu_pipe(void)
{
    unsigned long   dwLockFlag = 0;
	unsigned short dwReadData = 0;

	spin_lock_irqsave(&chicago_l2prefetch_lock, dwLockFlag);
	//toggle the flush miu pipe fire bit
	*(volatile unsigned short *)(0xFD002114) = 0x0;
	*(volatile unsigned short *)(0xFD002114) = 0x1;

	do
	{
		dwReadData = *(volatile unsigned short *)(0xFD002140);
		dwReadData &= BIT(12);  //Check Status of Flush Pipe Finish

	} while(dwReadData == 0);

	spin_unlock_irqrestore(&chicago_l2prefetch_lock, dwLockFlag);
}

static u64 chicago_phys_to_MIU(u64 x)
{

    return ((x) - CHICAGO_MIU0_BASE);
}

static u64 chicago_MIU_to_phys(u64 x)
{

    return ((x) + CHICAGO_MIU0_BASE);
}


struct soc_device_attribute chicago_soc_dev_attr;

extern const struct of_device_id of_default_bus_match_table[];

static int chicago_get_boot_type(void)
{
    //spade: to be finished...
    return 0;
}

static char chicago_platform_name[]=PLATFORM_NAME_CHICAGO;

char* chicago_get_platform_name(void)
{
    return chicago_platform_name;
}

static unsigned long long chicago_chip_get_riu_phys(void)
{
	return IO_PHYS;
}

static int chicago_chip_get_riu_size(void)
{
	return IO_SIZE;
}


int chicago_chip_function_set(int function_id, int param)
{
	int res=-1;

	printk("[%s]CHIP_FUNCTION SET. ID=%d, param=%d\n", PLATFORM_NAME, function_id, param);

	switch (function_id)
	{
			case CHIP_FUNC_UART_ENABLE_LINE:
				chicago_uart_enable_line(param);
				break;
			case CHIP_FUNC_UART_DISABLE_LINE:
				chicago_uart_disable_line(param);
				break;

		default:
			printk(KERN_ERR"[%s]Unsupport CHIP_FUNCTION!! ID=%d\n", PLATFORM_NAME, function_id);

	}



	return res;
}

static void __init chicago_init_early(void)
{
    struct ms_chip *chip=NULL;

    ms_chip_init_default();

    chip=ms_chip_get();


    chip->chip_flush_miu_pipe=chicago_chip_flush_miu_pipe;
    chip->phys_to_miu=chicago_phys_to_MIU;
    chip->miu_to_phys=chicago_MIU_to_phys;
    chip->chip_get_device_id=chicago_get_device_id;
    chip->chip_get_revision=chicago_get_revision;
    chip->chip_get_boot_dev_type=chicago_get_boot_type;
    chip->chip_get_platform_name=chicago_get_platform_name;
    chip->chip_get_riu_phys=chicago_chip_get_riu_phys;
    chip->chip_get_riu_size=chicago_chip_get_riu_size;

    chip->chip_function_set=chicago_chip_function_set;
}

static void __init chicago_init_machine(void)
{
    struct soc_device *soc_dev;
    struct device *parent = NULL;

    pr_info("[chicago_init_machine]\n");

    chicago_soc_dev_attr.family = kasprintf(GFP_KERNEL, chicago_platform_name);
    chicago_soc_dev_attr.revision = kasprintf(GFP_KERNEL, "%d", chicago_get_revision());
    chicago_soc_dev_attr.soc_id = kasprintf(GFP_KERNEL, "%u", chicago_get_device_id());

    soc_dev = soc_device_register(&chicago_soc_dev_attr);
    if (IS_ERR(soc_dev)) 
    {
        kfree((void *)chicago_soc_dev_attr.family);
        kfree((void *)chicago_soc_dev_attr.revision);
        kfree((void *)chicago_soc_dev_attr.soc_id);
        goto out;
    }

    parent = soc_device_to_device(soc_dev);

        /*
         * Finished with the static registrations now; fill in the missing
         * devices
         */
out:
    of_platform_populate(NULL, of_default_bus_match_table, NULL, parent);
}


DT_MACHINE_START(MS_CHICAGO_DT, "MStar Chicago (Flattened Device Tree)")
    .dt_compat    = chicago_dt_compat,
    .map_io = chicago_map_io,
    .init_machine = chicago_init_machine,
    .init_early = chicago_init_early,
    .restart = chicago_restart,
//    .init_time =  ms_init_timer,
//    .init_irq = chicago_init_irq,
    .nr_irqs = CHICAGO_NR_IRQS,
MACHINE_END
