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
#include <asm/hardware/cache-l2x0.h>

#include "cedric/registers.h"
#include "ms_platform.h"
#include "ms_types.h"
#include "_ms_private.h"

#define CEDRIC_MIU0_BASE               0x40000000

#define IO_PHYS         		0x1F000000
#define IO_VIRT                 (IO_PHYS+IO_OFFSET)//from IO_ADDRESS(x)
#define IO_OFFSET       		(MS_IO_OFFSET)
#define IO_SIZE         		0x00400000

#define PERI_PHYS         		0x16000000
#define PERI_VIRT         		(PERI_PHYS+PERI_OFFSET) //from IO_ADDRESS(x)
#define PERI_OFFSET       		(MS_IO_OFFSET)
#define PERI_SIZE         		0x2000

#define L2_CACHE_PHYS			0x15000000
#define L2_CACHE_VIRT         	(L2_CACHE_PHYS+L2_CACHE_OFFSET)
#define L2_CACHE_OFFSET       	(MS_IO_OFFSET)
#define L2_CACHE_SIZE         	0x1000

#define MS_DEVINFO_PHYS			0x40000000
#define MS_DEVINFO_VIRT         0xFE000000
#define MS_DEVINFO_OFFSET       (MS_DEVINFO_VIRT-MS_DEVINFO_PHYS)
#define MS_DEVINFO_SIZE         0x100000

//#define IMI_PHYS         		0xE0000000
//#define IMI_VIRT       			0xF9000000
//#define INI_OFFSET              (IMI_VIRT-IMI_PHYS)
//#define IMI_SIZE         		0x50000



#define L2_LINEFILL 0
#define L2_PREFETCH 0

extern struct ms_chip* ms_chip_get(void);
extern void __init ms_chip_init_default(void);
static void __iomem * l2x0_base = (void __iomem *)(IO_ADDRESS(L2_CACHE_PHYS));

extern void cedric_reset_cpu(int i);

#define PLATFORM_NAME PLATFORM_NAME_CEDRIC

static void cedric_restart(enum reboot_mode mode, const char *cmd)
{
        cedric_reset_cpu(0);
}


/* IO tables */
static struct map_desc cedric_io_desc[] __initdata =
{
    /* Define Registers' physcial and virtual addresses */
    {
        .virtual        = IO_VIRT,
        .pfn            = __phys_to_pfn(IO_PHYS),
        .length         = IO_SIZE,
        .type           = MT_DEVICE
    },

    /* Define periphral physcial and virtual addresses */
    {
	    .virtual        = PERI_VIRT,
	    .pfn            = __phys_to_pfn(PERI_PHYS),
	    .length         = PERI_SIZE,
	    .type           = MT_DEVICE
    },

    {
	    .virtual	    = L2_CACHE_VIRT,
	    .pfn	    	= __phys_to_pfn(L2_CACHE_PHYS),
	    .length	    	= L2_CACHE_SIZE,
	    .type	    	= MT_DEVICE,
    },

    {
	    .virtual	    = MS_DEVINFO_VIRT,
	    .pfn	    	= __phys_to_pfn(MS_DEVINFO_PHYS),
	    .length	    	= MS_DEVINFO_SIZE,
	    .type	    	= MT_DEVICE,
    },



//    /*IMI physical and virtual*/
//    {
//	    .virtual	    = IMI_VIRT,
//	    .pfn	    	= __phys_to_pfn(IMI_PHYS),
//	    .length	    	= IMI_SIZE,
//	    .type	    	= MT_DEVICE,
//    },

};




typedef struct _DEVINFO{
    unsigned char  header[8];
    unsigned short board_name;
    unsigned char  package_name;
    unsigned char  miu0_type;
    unsigned char  miu0_size;
    unsigned char  miu1_type;
    unsigned char  miu1_size;
    unsigned char  panel_type;
    unsigned char  rtk_flag;
    unsigned char  boot_device;
    unsigned char  uartpad_select;
    unsigned char  cpu_speed;
    unsigned char  rom_flag;
    unsigned char  customer_flag[4];
    unsigned char  rtk_qst_cfg;
}DEVINFO_st;

//address layout
#define DEVINFO_PHYS_ADDR     0x4003F800
//#define DEVINFO_SIZE             	0x800


//static MS_MEMCFG memcfg;
static MS_BOOT_DEV_TYPE cedric_boot_type;


static const char *cedric_dt_compat[] __initconst = {
	"mstar,cedric",
	NULL,
};

static void __init cedric_map_io(void)
{
    iotable_init(cedric_io_desc, ARRAY_SIZE(cedric_io_desc));

}

/*************************************
*		Mstar chip flush function
*************************************/
#define _BIT(x)                      (1<<(x))

static DEFINE_SPINLOCK(cedric_l2prefetch_lock);

static void cedric_uart_disable_line(int line)
{
    if(line == 0)
    {
    	CLRREG16(BASE_REG_PMMISC_PA+REG_ID_09, BIT11);
    }
    else if(line == 1)
    {
        CLRREG16(BASE_REG_CHIPGPIO1_PA + REG_ID_03,BIT2);
    }
    else if(line==2)
    {
        CLRREG16(BASE_REG_CHIPGPIO1_PA + REG_ID_03,BIT3);
    }
    else if(line==3)
    {
        CLRREG16(BASE_REG_CHIPGPIO1_PA + REG_ID_03,BIT4);
    }
    else if(line==4)
    {
        CLRREG16(BASE_REG_CHIPGPIO1_PA + REG_ID_03,BIT5);
    }
    else if(line==5)
    {
        CLRREG16(BASE_REG_CHIPGPIO1_PA + REG_ID_03,BIT6);
    }

}
static void cedric_uart_enable_line(int line)
{
    if(line == 0)
    {
    	SETREG16(BASE_REG_PMMISC_PA+REG_ID_09, BIT11);
    }
    else if(line == 1)
    {
        if((INREG16(BASE_REG_CHIPTOP_PA + REG_ID_53)& 0x00F0) == 0x00F0)
        {
            OUTREG16(BASE_REG_CHIPTOP_PA + REG_ID_53,((INREG16(BASE_REG_CHIPTOP_PA + REG_ID_53)& 0xFF0F)|0x0030));
        }
        SETREG16(BASE_REG_CHIPGPIO1_PA + REG_ID_03,BIT2);
    }
    else if(line==2)
    {
        if((INREG16(BASE_REG_CHIPTOP_PA + REG_ID_53)& 0x0F00) == 0x0F00)
        {
            OUTREG16(BASE_REG_CHIPTOP_PA + REG_ID_53,((INREG16(BASE_REG_CHIPTOP_PA + REG_ID_53)& 0xF0FF)|0x0400));
        }
        SETREG16(BASE_REG_CHIPGPIO1_PA + REG_ID_03,BIT3);
    }
    else if(line==3)
    {
        if((INREG16(BASE_REG_CHIPTOP_PA + REG_ID_53)& 0xF000) == 0xF000)
        {
            OUTREG16(BASE_REG_CHIPTOP_PA + REG_ID_53,((INREG16(BASE_REG_CHIPTOP_PA + REG_ID_53)& 0x0FFF)|0x5000));
        }
        SETREG16(BASE_REG_CHIPGPIO1_PA + REG_ID_03,BIT4);
    }
    else if(line==4)
    {
        if((INREG16(BASE_REG_CHIPTOP_PA + REG_ID_54)& 0x000F) == 0x000F)
        {
            OUTREG16(BASE_REG_CHIPTOP_PA + REG_ID_54,((INREG16(BASE_REG_CHIPTOP_PA + REG_ID_54)& 0xFFF0)|0x0000));
        }
        SETREG16(BASE_REG_CHIPGPIO1_PA + REG_ID_03,BIT5);
    }
    else if(line==5)
    {
        if((INREG16(BASE_REG_CHIPTOP_PA + REG_ID_54)& 0x00F0) == 0x00F0)
        {
            OUTREG16(BASE_REG_CHIPTOP_PA + REG_ID_54,((INREG16(BASE_REG_CHIPTOP_PA + REG_ID_54)& 0xFF0F)|0x0010));
        }
        SETREG16(BASE_REG_CHIPGPIO1_PA + REG_ID_03,BIT6);
    }

}

static int cedric_get_device_id(void)
{
	return (int)(INREG16(0x1F003C00) & 0x00FF);
}

static int cedric_get_revision(void)
{
	u16 tmp = 0;
		tmp = INREG16((unsigned int)(0x1F003C00+0x67*4));
	tmp=((tmp >> 8) & 0x00FF);

	return (tmp+1);
}


static int __init cedric_early_devinfo_config(void)
{
	DEVINFO_st *devinfo;
	devinfo = (DEVINFO_st*)(DEVINFO_PHYS_ADDR+MS_DEVINFO_OFFSET);    // start address: 30K, size:  2K
	// check header

	if (devinfo->header[0]!='D' || devinfo->header[1]!='E' || devinfo->header[2]!='V' || devinfo->header[3]!='I' ||
		devinfo->header[4]!='N' || devinfo->header[5]!='F' || devinfo->header[6]!='O' || devinfo->header[7]!='.')
	{
		pr_info("***********************************************************\n");
		pr_info("* [MS:DEVINFO] header check failed !!!                    *\n");
		pr_info("* Please program DEVINFO section.                         *\n");
		pr_info("***********************************************************\n");

	}
	else
	{
//		unsigned int m0size=0;
//        unsigned int i;

		pr_info("[MS:DEVINFO] header check passed !!\n");



		cedric_boot_type=(MS_BOOT_DEV_TYPE)devinfo->boot_device;
//		panel_type=(DEVINFO_PANEL_TYPE)devinfo->panel_type;
//		board_type = (DEVINFO_BOARD_TYPE)devinfo->board_name;
//        rtk_flag = (DEVINFO_RTK_FLAG)devinfo->rtk_flag;
//        rtk_qst_cfg = devinfo->rtk_qst_cfg;

//        rtkinfo_init();

	}

	return 0;

}


static void cedric_chip_flush_miu_pipe(void)
{
	unsigned long   dwLockFlag = 0;
	unsigned short dwReadData = 0;

	spin_lock_irqsave(&cedric_l2prefetch_lock, dwLockFlag);
	//toggle the flush miu pipe fire bit
	*(volatile unsigned short *)(0xFD203114) = 0x0;
	*(volatile unsigned short *)(0xFD203114) = 0x1;

	do
	{
		dwReadData = *(volatile unsigned short *)(0xFD203140);
		dwReadData &= _BIT(12);  //Check Status of Flush Pipe Finish

	} while(dwReadData == 0);

	spin_unlock_irqrestore(&cedric_l2prefetch_lock, dwLockFlag);

}

static u64 cedric_phys_to_MIU(u64 x)
{

	return ((x) - CEDRIC_MIU0_BASE);
}

static u64 cedric_MIU_to_phys(u64 x)
{

	return ((x) + CEDRIC_MIU0_BASE);
}


static int cedric_outer_cache_is_enable(void)
{

    return (readl_relaxed(l2x0_base + L2X0_CTRL) & 1);
}

struct soc_device_attribute cedric_soc_dev_attr;
extern struct outer_cache_fns outer_cache;
extern const struct of_device_id of_default_bus_match_table[];

static int cedric_get_boot_type(void)
{
	return (int)cedric_boot_type;
}

static char cedric_platform_name[]=PLATFORM_NAME_CEDRIC;

static char* cedric_get_platform_name(void)
{
	return cedric_platform_name;
}


static unsigned long long cedric_chip_get_riu_phys(void)
{
	return IO_PHYS;
}

static int cedric_chip_get_riu_size(void)
{
	return IO_SIZE;
}


// Define PM_GPIO registers
#define REG_PM_GPIO_BASE      0xFD001E00
#define REG_PM_GPIO_00H		(REG_PM_GPIO_BASE + 0x0000*4)

#define REG_PM_MISC_BASE      0xFD005C00
#define REG_PM_MISC_21H		(REG_PM_MISC_BASE + 0x0021*4)

static int cedric_ir_enable(int param)
{


    (*(volatile u32 *)(REG_PM_GPIO_00H)) |= 0x0008;
	(*(volatile u32 *)(REG_PM_MISC_21H)) &= 0xfe1f;

	return 0;
}

static int cedric_chip_function_set(int function_id, int param)
{
	int res=0;

	printk("[%s]CHIP_FUNCTION SET. ID=%d, param=%d\n",PLATFORM_NAME,function_id,param);
	switch (function_id)
	{

		case CHIP_FUNC_UART_ENABLE_LINE:
			cedric_uart_enable_line(param);
			break;
		case CHIP_FUNC_UART_DISABLE_LINE:
			cedric_uart_disable_line(param);
			break;
		case CHIP_FUNC_IR_ENABLE:
			cedric_ir_enable(param);
			break;
		default:
			res=-1;
			printk(KERN_ERR"[%s]Unsupport CHIP_FUNCTION!! ID=%d\n",PLATFORM_NAME,function_id);

	}

	return res;
}

static void __init cedric_init_early(void)
{
	struct ms_chip *chip=NULL;

	ms_chip_init_default();

	cedric_early_devinfo_config();

	chip=ms_chip_get();


	chip->chip_flush_miu_pipe=cedric_chip_flush_miu_pipe;
	chip->phys_to_miu=cedric_phys_to_MIU;
	chip->miu_to_phys=cedric_MIU_to_phys;

	chip->cache_outer_is_enabled=cedric_outer_cache_is_enable;
//

	chip->chip_get_device_id=cedric_get_device_id;
	chip->chip_get_revision=cedric_get_revision;
	chip->chip_get_boot_dev_type=cedric_get_boot_type;
	chip->chip_get_platform_name=cedric_get_platform_name;

	chip->chip_get_riu_phys=cedric_chip_get_riu_phys;
	chip->chip_get_riu_size=cedric_chip_get_riu_size;
	chip->chip_function_set=cedric_chip_function_set;

}



extern MS_VERSION LX_VERSION;
static void __init cedric_init_machine(void)
{
	struct soc_device *soc_dev;
	struct device *parent = NULL;

	char ver[33]={0};
	memcpy(ver,(void *)&LX_VERSION,32);

	pr_info("[cedric_init_machine]");
	pr_info("%s\n",ver);



#ifdef CONFIG_MS_L2X0_PATCH
	outer_cache.flush_MIU_pipe=&cedric_chip_flush_miu_pipe;
#endif


#ifdef CONFIG_CACHE_L2X0
	{
		//void __iomem * l2x0_base = (void __iomem *)(IO_ADDRESS(L2_CACHE_PHYS));
#if L2_LINEFILL
		val = L2_CACHE_read( L2_CACHE_PHYS + PREFETCH_CTL_REG );
		L2_CACHE_write(( val | DOUBLE_LINEFILL_ENABLE | LINEFILL_WRAP_DISABLE ), L2_CACHE_PHYS + PREFETCH_CTL_REG);
#endif

#if L2_PREFETCH
		val = L2_CACHE_read( L2_CACHE_PHYS + PREFETCH_CTL_REG );
		L2_CACHE_write(( val | I_PREFETCH_ENABLE | D_PREFETCH_ENABLE | PREFETCH_OFFSET ), L2_CACHE_PHYS + PREFETCH_CTL_REG );
#endif

		/* set RAM latencies to 1 cycle for this core tile. */
		writel(0x113, l2x0_base + L310_TAG_LATENCY_CTRL);
		writel(0x111, l2x0_base + L310_DATA_LATENCY_CTRL);

		l2x0_init(l2x0_base, 0x00400000, 0xfe0fffff);
	}
#endif





		cedric_soc_dev_attr.family = kasprintf(GFP_KERNEL, cedric_platform_name);
		cedric_soc_dev_attr.revision = kasprintf(GFP_KERNEL, "%d", cedric_get_revision());
		cedric_soc_dev_attr.soc_id = kasprintf(GFP_KERNEL, "%u", cedric_get_device_id());
		cedric_soc_dev_attr.api_version = kasprintf(GFP_KERNEL, ms_chip_get()->chip_get_API_version());

		soc_dev = soc_device_register(&cedric_soc_dev_attr);
		if (IS_ERR(soc_dev)) {
			kfree((void *)cedric_soc_dev_attr.family);
			kfree((void *)cedric_soc_dev_attr.revision);
			kfree((void *)cedric_soc_dev_attr.soc_id);
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






#ifdef CONFIG_SMP
extern struct smp_operations cedric_smp_ops;
#endif

DT_MACHINE_START(MS_CEDRIC_DT, "MStar Cedric (Flattened Device Tree)")
	.dt_compat	= cedric_dt_compat,
	.map_io =cedric_map_io,
	.init_machine = cedric_init_machine,
	.init_early = cedric_init_early,
	.restart    = cedric_restart,
#ifdef CONFIG_SMP
	.smp		= smp_ops(cedric_smp_ops),
#endif
//	.init_time =  ms_init_timer,
//
MACHINE_END
