#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/irqchip.h>
#include <linux/of_platform.h>
#include <linux/of_fdt.h>
#include <linux/sys_soc.h>
#include <linux/slab.h>
#include <linux/suspend.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/clocksource.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/memory.h>
#include <asm/io.h>
#include <asm/mach/map.h>

#include "infinity/registers.h"
#include "infinity/mcm_id.h"
#include "ms_platform.h"
#include "ms_types.h"
#include "_ms_private.h"

/* IO tables */
static struct map_desc infinity_io_desc[] __initdata =
{
    /* Define Registers' physcial and virtual addresses */
        {IO_VIRT,   __phys_to_pfn(IO_PHYS),     IO_SIZE,        MT_DEVICE},
        {SPI_VIRT,  __phys_to_pfn(SPI_PHYS),    SPI_SIZE,       MT_DEVICE},
        {GIC_VIRT,  __phys_to_pfn(GIC_PHYS),    GIC_SIZE,       MT_DEVICE},
        //{IMI_VIRT,  __phys_to_pfn(IMI_PHYS),    IMI_SIZE,       MT_DEVICE},
};


static const char *infinity_dt_compat[] __initconst = {
    "mstar,infinity",
    NULL,
};

static void __init infinity_map_io(void)
{
    iotable_init(infinity_io_desc, ARRAY_SIZE(infinity_io_desc));
}

#define PLATFORM_NAME PLATFORM_NAME_INFINITY

extern struct ms_chip* ms_chip_get(void);
extern void __init ms_chip_init_default(void);
//extern void __init infinity_init_irqchip(void);
extern void infinity_restart(enum reboot_mode mode, const char *cmd);
extern struct timecounter *arch_timer_get_timecounter(void);


static int mcm_rw(int index, int ratio, int write);


/*************************************
*        Mstar chip flush function
*************************************/
static DEFINE_SPINLOCK(infinity_l2prefetch_lock);

static void infinity_uart_disable_line(int line)
{
    if(line == 0)  //for debug, do not change
    {
        // UART0_Mode -> X
        //CLRREG16(BASE_REG_CHIPTOP_PA + REG_ID_03, BIT4 | BIT5);
        //CLRREG16(BASE_REG_PMSLEEP_PA + REG_ID_09, BIT11);
    }
    else if(line == 1)
    {
        // UART1_Mode -> X
        CLRREG16(BASE_REG_CHIPTOP_PA + REG_ID_03, BIT8 | BIT9);
    }
    else if(line == 2)
    {
        // FUART_Mode -> X
        CLRREG16(BASE_REG_CHIPTOP_PA + REG_ID_03, BIT0 | BIT1);
    }
}

static void infinity_uart_enable_line(int line)
{
    if(line == 0)  //for debug, do not change
    {
        // UART0_Mode -> PAD_UART0_TX/RX
        //SETREG16(BASE_REG_CHIPTOP_PA + REG_ID_03, BIT4);
    }
    else if(line == 1)
    {
        // UART1_Mode -> PAD_UART1_TX/RX
        SETREG16(BASE_REG_CHIPTOP_PA + REG_ID_03, BIT8);
    }
    else if(line==2)
    {
        // FUART_Mode -> PAD_FUART_TX/RX
        SETREG16(BASE_REG_CHIPTOP_PA + REG_ID_03, BIT0);
    }
}

static int infinity_get_device_id(void)
{
    return (int)(INREG16(0x1F003C00) & 0x00FF);;
}

static int infinity_get_revision(void)
{
    u16 tmp = 0;
    tmp = INREG16((unsigned int)(BASE_REG_PMTOP_PA + REG_ID_67));
    tmp=((tmp >> 8) & 0x00FF);

    return (tmp+1);
}


static void infinity_chip_flush_miu_pipe(void)
{
    unsigned long   dwLockFlag = 0;
    unsigned short dwReadData = 0;

    spin_lock_irqsave(&infinity_l2prefetch_lock, dwLockFlag);
    //toggle the flush miu pipe fire bit
    *(volatile unsigned short *)(0xFD204414) = 0x0;
    *(volatile unsigned short *)(0xFD204414) = 0x1;

    do
    {
        dwReadData = *(volatile unsigned short *)(0xFD204440);
        dwReadData &= BIT12;  //Check Status of Flush Pipe Finish

    } while(dwReadData == 0);

    spin_unlock_irqrestore(&infinity_l2prefetch_lock, dwLockFlag);

}

static u64 infinity_phys_to_MIU(u64 x)
{

    return ((x) - INFINITY_MIU0_BASE);
}

static u64 infinity_MIU_to_phys(u64 x)
{

    return ((x) + INFINITY_MIU0_BASE);
}


struct soc_device_attribute infinity_soc_dev_attr;

extern const struct of_device_id of_default_bus_match_table[];

static int infinity_get_storage_type(void)
{
    u8 type = ((INREG16(BASE_REG_DIDKEY_PA + 0x70) >> 4) & 0x3);

    if(1 == type)
        return (int)MS_STORAGE_NAND;
    else if(2 == type)
        return (int)MS_STORAGE_NOR;
    else
        return (int)MS_STORAGE_UNKNOWN;
}

static int infinity_get_package_type(void)
{
    if(!strcmp(&infinity_soc_dev_attr.machine[8], "MSC316D"))
        return MS_PACKAGE_BGA;
    else if(!strcmp(&infinity_soc_dev_attr.machine[8], "MSC316Q"))
        return MS_PACKAGE_BGA_256M;
    else if(!strcmp(&infinity_soc_dev_attr.machine[8], "MSC315"))
        return MS_PACKAGE_QFP;
    else if(!strcmp(&infinity_soc_dev_attr.machine[8], "MSC313"))
        return MS_PACKAGE_QFN;
    else
    {
        printk(KERN_ERR "** ERROR ** Machine name [%s] not support\n", infinity_soc_dev_attr.machine);
        return MS_PACKAGE_UNKNOWN;
    }
}
static char infinity_platform_name[]=PLATFORM_NAME_INFINITY;

char* infinity_get_platform_name(void)
{
    return infinity_platform_name;
}

static unsigned long long infinity_chip_get_riu_phys(void)
{
    return IO_PHYS;
}

static int infinity_chip_get_riu_size(void)
{
    return IO_SIZE;
}


static int infinity_ir_enable(int param)
{
    printk(KERN_ERR "NOT YET IMPLEMENTED!![%s]",__FUNCTION__);
    return 0;
}


static int infinity_usb_vbus_control(int param)
{
    int package = infinity_get_package_type();

    if(0 == param) //disable vbus
    {
        if(MS_PACKAGE_BGA == package || MS_PACKAGE_BGA_256M == package)
        {
            // BGA (PAD_PM_GPIO2)
            CLRREG8(BASE_REG_PMSLEEP_PA + REG_ID_28, BIT6|BIT7);  //reg_pwm2_mode = 0
            CLRREG8(BASE_REG_PMGPIO_PA + REG_ID_02, BIT1);  //output = 0
            CLRREG8(BASE_REG_PMGPIO_PA + REG_ID_02, BIT0);  //oen = 0
        }
        else if(MS_PACKAGE_QFP == package || MS_PACKAGE_QFN == package)
        {
            // QFP (PAD_SPI0_CK) need to check many register, please refer to GPIO table
            CLRREG8(0x1F2079C4, BIT4);  //output = 0
            CLRREG8(0x1F2079C4, BIT5);  //oen = 0
        }
        else
        {
            printk(KERN_ERR "[%s] TODO: package type =%d\n", __FUNCTION__, package);
            return 0;
        }
        printk(KERN_INFO "[%s] Disable USB VBUS\n", __FUNCTION__);
    }
    else if(1 == param)
    {
        if(MS_PACKAGE_BGA == package || MS_PACKAGE_BGA_256M == package)
        {
            // BGA (PAD_PM_GPIO2)
            CLRREG8(BASE_REG_PMSLEEP_PA + REG_ID_28, BIT6|BIT7);  //reg_pwm2_mode = 0
            SETREG8(BASE_REG_PMGPIO_PA + REG_ID_02, BIT1);  //output = 1
            CLRREG8(BASE_REG_PMGPIO_PA + REG_ID_02, BIT0);  //oen = 0
        }
        else if(MS_PACKAGE_QFP == package || MS_PACKAGE_QFN == package)
        {
            // QFP (PAD_SPI0_CK) need to check many register, please refer to GPIO table
            SETREG8(0x1F2079C4, BIT4);  //output = 1
            CLRREG8(0x1F2079C4, BIT5);  //oen = 0
        }
        else
        {
            printk(KERN_ERR "[%s] TODO: package type =%d\n", __FUNCTION__, package);
            return 0;
        }
        printk(KERN_INFO "[%s] Enable USB VBUS\n", __FUNCTION__);
    }
    else
    {
        printk(KERN_ERR "[%s] param invalid\n", __FUNCTION__);
    }
    return 0;
}

extern u32 arch_timer_get_rate(void);
static cycle_t us_ticks_cycle_offset=0;
static u64 us_ticks_factor=1;

static u64 infinity_chip_get_us_ticks(void)
{
	const struct cyclecounter *arch_cc=arch_timer_get_timecounter()->cc;
	u64 cycles=(arch_cc->read(arch_cc)-us_ticks_cycle_offset);
	u64 usticks=div64_u64(cycles,us_ticks_factor);
	return usticks;
}

void intinify_reset_us_ticks_cycle_offset(void)
{
	const struct cyclecounter *arch_cc=arch_timer_get_timecounter()->cc;
	us_ticks_cycle_offset=arch_cc->read(arch_cc);
}

static int infinity_chip_function_set(int function_id, int param)
{
    int res=-1;

    printk("[%s]CHIP_FUNCTION SET. ID=%d, param=%d\n",PLATFORM_NAME,function_id,param);
    switch (function_id)
    {
            case CHIP_FUNC_UART_ENABLE_LINE:
                infinity_uart_enable_line(param);
                break;
            case CHIP_FUNC_UART_DISABLE_LINE:
                infinity_uart_disable_line(param);
                break;
            case CHIP_FUNC_IR_ENABLE:
                infinity_ir_enable(param);
                break;
            case CHIP_FUNC_USB_VBUS_CONTROL:
                infinity_usb_vbus_control(param);
                break;
            case CHIP_FUNC_MCM_DISABLE_ID:
                mcm_rw(param, 0, 1);
                break;
            case CHIP_FUNC_MCM_ENABLE_ID:
                mcm_rw(param, 15, 1);
                break;
        default:
            printk(KERN_ERR "[%s]Unsupport CHIP_FUNCTION!! ID=%d\n",PLATFORM_NAME,function_id);

    }

    return res;
}


static void __init infinity_init_early(void)
{


    struct ms_chip *chip=NULL;
    ms_chip_init_default();

    chip=ms_chip_get();


    chip->chip_flush_miu_pipe=infinity_chip_flush_miu_pipe;
    chip->phys_to_miu=infinity_phys_to_MIU;
    chip->miu_to_phys=infinity_MIU_to_phys;
    chip->chip_get_device_id=infinity_get_device_id;
    chip->chip_get_revision=infinity_get_revision;
    chip->chip_get_platform_name=infinity_get_platform_name;
    chip->chip_get_riu_phys=infinity_chip_get_riu_phys;
    chip->chip_get_riu_size=infinity_chip_get_riu_size;

    chip->chip_function_set=infinity_chip_function_set;
    chip->chip_get_storage_type=infinity_get_storage_type;
    chip->chip_get_package_type=infinity_get_package_type;
    chip->chip_get_us_ticks=infinity_chip_get_us_ticks;

}

extern char* LX_VERSION;
static void __init infinity_init_machine(void)
{
    struct soc_device *soc_dev;
    struct device *parent = NULL;

    pr_info("\n\n[INFINITY] : %s\n\n",LX_VERSION);

    intinify_reset_us_ticks_cycle_offset();
    us_ticks_factor=div64_u64(arch_timer_get_rate(),1000000);

    infinity_soc_dev_attr.family = kasprintf(GFP_KERNEL, infinity_platform_name);
    infinity_soc_dev_attr.revision = kasprintf(GFP_KERNEL, "%d", infinity_get_revision());
    infinity_soc_dev_attr.soc_id = kasprintf(GFP_KERNEL, "%u", infinity_get_device_id());
    infinity_soc_dev_attr.api_version = kasprintf(GFP_KERNEL, ms_chip_get()->chip_get_API_version());
    infinity_soc_dev_attr.machine = kasprintf(GFP_KERNEL, of_flat_dt_get_machine_name());

    soc_dev = soc_device_register(&infinity_soc_dev_attr);
    if (IS_ERR(soc_dev)) {
        kfree((void *)infinity_soc_dev_attr.family);
        kfree((void *)infinity_soc_dev_attr.revision);
        kfree((void *)infinity_soc_dev_attr.soc_id);
        kfree((void *)infinity_soc_dev_attr.machine);
        goto out;
    }

    parent = soc_device_to_device(soc_dev);

    /*
     * Finished with the static registrations now; fill in the missing
     * devices
     */
out:
    of_platform_populate(NULL, of_default_bus_match_table, NULL, parent);

    //disable ETAG pad setting if QFP or QFN package
    if(MS_PACKAGE_QFP == infinity_get_package_type() || MS_PACKAGE_QFN == infinity_get_package_type())
        CLRREG8(BASE_REG_CHIPTOP_PA + REG_ID_0F, BIT0|BIT1);
}




struct miu_device {
    struct device dev;
    int index;
};

struct miu_client{
    char* name;
    short bw_client_id;
    short bw_enabled;
    short bw_val;
    short bw_val_thread;
};

static struct miu_client miu0_clients[] = {
        {"OVERALL   ",0x00,0,0,0},
        {"MFE(F)    ",0x01,0,0,0},
        {"MFE(B)    ",0x02,0,0,0},
        {"VHE       ",0x03,0,0,0},
        {"JPE1      ",0x04,0,0,0},
        {"JPE0      ",0x05,0,0,0},
        {"BACH      ",0x06,0,0,0},
        {"FILE      ",0x07,0,0,0},
        {"UHC0      ",0x08,0,0,0},
        {"EMAC      ",0x09,0,0,0},
        {"MCU51     ",0x0A,0,0,0},
        {"URDMA     ",0x0B,0,0,0},
        {"BDMA      ",0x0C,0,0,0},
        {"NA        ",0x0D,0,0,0},
        {"NA        ",0x0E,0,0,0},
        {"NA        ",0x0F,0,0,0},
        {"CMDQ      ",0x10,0,0,0},
        {"ISP_DNR   ",0x11,0,0,0},
        {"ISP_ROT   ",0x12,0,0,0},
        {"ISP_DMA   ",0x13,0,0,0},
        {"ISP_STA   ",0x14,0,0,0},
        {"GOP       ",0x15,0,0,0},
        {"SC_DNR    ",0x16,0,0,0},
        {"SC_DNR_SAD",0x17,0,0,0},
        {"SC_CROP   ",0x18,0,0,0},
        {"SC1_FRM   ",0x19,0,0,0},
        {"SC1_SNP   ",0x1A,0,0,0},
        {"SC1_DBG   ",0x1B,0,0,0},
        {"SC2_FRM   ",0x1C,0,0,0},
        {"SC3_FRM   ",0x1D,0,0,0},
        {"FCIE      ",0x1E,0,0,0},
        {"SDIO      ",0x1F,0,0,0},
        {"CPU       ",0x70,0,0,0},

};

static struct miu_device miu0;


static struct bus_type miu_subsys = {
    .name = "miu",
    .dev_name = "miu",
};

static struct task_struct *pBWmonitorThread=NULL;
struct mutex bw_monitor_mutex;


int BW_measure(short bwclientid)
{
    short BW_val=0;
    mutex_lock(&bw_monitor_mutex);
    OUTREG16( (BASE_REG_MIU_PA+REG_ID_0D), ( ((bwclientid << 8) & 0xFF00) | 0x50)) ;//reset
    OUTREG16( (BASE_REG_MIU_PA+REG_ID_0D), ( ((bwclientid << 8) & 0xFF00) | 0x51)) ;//set to read peak

    mdelay(300);
    BW_val=INREG16((BASE_REG_MIU_PA+REG_ID_0E));

    OUTREG16( (BASE_REG_MIU_PA+REG_ID_0D),0) ;//reset all

    mutex_unlock(&bw_monitor_mutex);

    return BW_val;
}

static int BW_monitor(void *arg)
{
    int i=0;
    short tempBW_val=0;
    while(1)
    {
        if (kthread_should_stop()) break;
        for(i=0; i<(sizeof(miu0_clients)/sizeof(miu0_clients[0]));i++)
        {

            if(miu0_clients[i].bw_enabled)
            {
                //OUTREG16( (BASE_REG_MIU_PA+REG_ID_0D), ( ((miu0_clients[i].bw_client_id << 8) & 0xFF00) | 0x50)) ;//reset
                //OUTREG16( (BASE_REG_MIU_PA+REG_ID_0D), ( ((miu0_clients[i].bw_client_id << 8) & 0xFF00) | 0x51)) ;//set to read peak

                //mdelay(300);
                //tempBW_val=0;
                //tempBW_val=INREG16((BASE_REG_MIU_PA+REG_ID_0E));

                tempBW_val=BW_measure(miu0_clients[i].bw_client_id);

                if(miu0_clients[i].bw_val_thread<tempBW_val)
                {
                    miu0_clients[i].bw_val_thread=tempBW_val;
                }
            }
        }

        //OUTREG16( (BASE_REG_MIU_PA+REG_ID_0D),0) ;//reset all
        mdelay(100);
    }

   return 0;

}


static int set_miu_client_enable(struct device *dev, const char *buf, size_t n, int enabled)
{
    long idx=-1;
    if (kstrtol(buf, 10, &idx) != 0 || idx<0 || idx >= (sizeof(miu0_clients)/sizeof(miu0_clients[0])) ) return -EINVAL;

    if('0'== (dev->kobj.name[3]))
    {
        miu0_clients[idx].bw_enabled=enabled;
    }
    return n;
}

static ssize_t bw_enable_store(struct device *dev,  struct device_attribute *attr, const char *buf, size_t n)
{
    return set_miu_client_enable(dev,buf,n,1);
}

static ssize_t bw_enable_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;
    int i=0;

    if('0'== (dev->kobj.name[3]))
    {
        for(i=0; i<(sizeof(miu0_clients)/sizeof(miu0_clients[0]));i++)
        {
            if(miu0_clients[i].bw_enabled)
            {
                str += scnprintf(str, end - str, "%d ",(short)i);
            }
        }
    }

    if (str > buf)  str--;

    str += scnprintf(str, end - str, "\n");

    return (str - buf);
}

static ssize_t bw_disable_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t n)
{
    return set_miu_client_enable(dev,buf,n,0);
}

static ssize_t bw_disable_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;
    int i=0;

    if('0'== (dev->kobj.name[3]))
    {
        str += scnprintf(str, end - str, "Num:IP_name   [BW_Idx][Enable(1)/Disable(0)]\n");
        for(i=0; i<(sizeof(miu0_clients)/sizeof(miu0_clients[0]));i++)
        {
            str += scnprintf(str, end - str, "%3d:%s[0x%04X][%d]\n",(short)i,miu0_clients[i].name,(short)miu0_clients[i].bw_client_id,(char)miu0_clients[i].bw_enabled);
        }
    }

    if (str > buf)  str--;

    str += scnprintf(str, end - str, "\n");

    return (str - buf);
}

static ssize_t bw_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t n)
{
    return 0;
}

static ssize_t bw_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;
    int ip_loop_time=1;
    int i=0, temp_loop_time=0;
    short tempBW_val=0;

    //reset all bw value
    for(i=0; i<(sizeof(miu0_clients)/sizeof(miu0_clients[0]));i++)
    {
        miu0_clients[i].bw_val=0;
    }

    for(i=0; i<(sizeof(miu0_clients)/sizeof(miu0_clients[0]));i++)
    {
        if(miu0_clients[i].bw_enabled)
        {
            for (temp_loop_time=0;temp_loop_time<ip_loop_time;temp_loop_time++)
            {
                //OUTREG16( (BASE_REG_MIU_PA+REG_ID_0D), ( ((miu0_clients[i].bw_client_id << 8) & 0xFF00) | 0x50)) ;//reset
                //OUTREG16( (BASE_REG_MIU_PA+REG_ID_0D), ( ((miu0_clients[i].bw_client_id << 8) & 0xFF00) | 0x51)) ;//set to read peak
                //mdelay(300);
                //tempBW_val=0;
                //tempBW_val=INREG16((BASE_REG_MIU_PA+REG_ID_0E));

                tempBW_val=BW_measure(miu0_clients[i].bw_client_id);

                if(miu0_clients[i].bw_val<tempBW_val)
                {
                    miu0_clients[i].bw_val=tempBW_val;
                }
            }
        }
        OUTREG16( (BASE_REG_MIU_PA+REG_ID_0D),0) ;//reset all
    }

    if('0'== (dev->kobj.name[3]))
    {
        for(i=0; i<(sizeof(miu0_clients)/sizeof(miu0_clients[0]));i++)
        {
            if(miu0_clients[i].bw_enabled)
            {
                //read from bw register and saved back to bw_val
                str += scnprintf(str, end - str, "%2d:%s[0x%04X] BW_val=%3d,%3d.%02d%%\n",(short)i,miu0_clients[i].name,
                (short)miu0_clients[i].bw_client_id,miu0_clients[i].bw_val,miu0_clients[i].bw_val*100/1024,
                (miu0_clients[i].bw_val*10000/1024)%100);

            }
        }
    }

    if (str > buf)  str--;

    str += scnprintf(str, end - str, "\n");

    return (str - buf);


}


static int set_bw_thread_enable(struct device *dev, const char *buf, size_t n)
{
    long idx=-1;
    int i=0;
    int ret;

    if (kstrtol(buf, 10, &idx) != 0 || idx<0 || idx >= 2 ) return -EINVAL;

    if(idx==1)//enable thread
    {
        for(i=0; i<(sizeof(miu0_clients)/sizeof(miu0_clients[0]));i++) //reset all bandwidth value
        {
            miu0_clients[i].bw_val_thread=0;
        }

        if(pBWmonitorThread==NULL)
        {
            pBWmonitorThread = kthread_create(BW_monitor,(void *)&pBWmonitorThread,"BW Monitor");
            if (IS_ERR(pBWmonitorThread))
            {
                ret = PTR_ERR(pBWmonitorThread);
                pBWmonitorThread = NULL;
                return ret;
            }
            wake_up_process(pBWmonitorThread);
        }
    }
    else if (idx==0 && (pBWmonitorThread!=NULL))//disable thread
    {
        kthread_stop(pBWmonitorThread);
    }
    return n;
}


static ssize_t bw_thread_store(struct device *dev,  struct device_attribute *attr, const char *buf, size_t n)
{
    return set_bw_thread_enable(dev,buf,n);
}


static ssize_t bw_thread_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;
    int i=0;

    if('0'== (dev->kobj.name[3]))
    {
        for(i=0; i<(sizeof(miu0_clients)/sizeof(miu0_clients[0]));i++)
        {
            if(miu0_clients[i].bw_enabled)
            {
                //read from bw register and saved back to bw_val
                str += scnprintf(str, end - str, "%2d:%s[0x%04X] BW_val_thread=%3d,%3d.%02d%%\n",(short)i,miu0_clients[i].name,
                (short)miu0_clients[i].bw_client_id,miu0_clients[i].bw_val_thread,miu0_clients[i].bw_val_thread*100/1024,
                (miu0_clients[i].bw_val_thread*10000/1024)%100);

            }
        }
    }

    if (str > buf)  str--;

    str += scnprintf(str, end - str, "\n");

    return (str - buf);


}


DEVICE_ATTR(bw_enable, 0644, bw_enable_show, bw_enable_store);
DEVICE_ATTR(bw_disable, 0644, bw_disable_show, bw_disable_store);
DEVICE_ATTR(bw, 0644, bw_show, bw_store);
DEVICE_ATTR(bw_thread, 0644, bw_thread_show, bw_thread_store);


static void __init infinity_create_MIU_node(void)
{
    int ret;

    miu0.index=0;
    miu0.dev.kobj.name="miu0";
    miu0.dev.bus=&miu_subsys;

    ret = subsys_system_register(&miu_subsys, NULL);
    if (ret)
    {
        printk(KERN_ERR "Failed to register miu sub system!! %d\n",ret);
        return;
    }


    ret=device_register(&miu0.dev);

    if(ret)
    {
        printk(KERN_ERR "Failed to register miu0 device!! %d\n",ret);
        return;
    }

    device_create_file(&miu0.dev, &dev_attr_bw_enable);
    device_create_file(&miu0.dev, &dev_attr_bw_disable);
    device_create_file(&miu0.dev, &dev_attr_bw);
    device_create_file(&miu0.dev, &dev_attr_bw_thread);
    mutex_init(&bw_monitor_mutex);
}

struct mcm_client{
    char* name;
    short index;
    short slow_down_ratio;
};


static struct mcm_client mcm_clients[] = {
    {"MCU51",         MCM_ID_MCU51, 0},
    {"URDMA",         MCM_ID_URDMA, 0},
    {"BDMA",          MCM_ID_BDMA, 0},
    {"VHE",           MCM_ID_VHE, 0},
    {"MFE",           MCM_ID_MFE, 0},
    {"JPE",           MCM_ID_JPE, 0},
    {"BACH",          MCM_ID_BACH, 0},
    {"AESDMA",        MCM_ID_AESDMA, 0},
    {"UHC",           MCM_ID_UHC, 0},
    {"EMAC",          MCM_ID_EMAC, 0},
    {"CMDQ",          MCM_ID_CMDQ, 0},
    {"ISP_DNR",       MCM_ID_ISP_DNR, 0},
    {"ISP_DMA",       MCM_ID_ISP_DMA, 0},
    {"GOP",           MCM_ID_GOP, 0},
    {"SC_DNR",        MCM_ID_SC_DNR, 0},
    {"SC_DNR_SAD",    MCM_ID_SC_DNR_SAD, 0},
    {"SC_CROP",       MCM_ID_SC_CROP, 0},
    {"SC1_FRM",       MCM_ID_SC1_FRM, 0},
    {"SC1_SNP",       MCM_ID_SC1_SNP, 0},
    {"SC1_DBG",       MCM_ID_SC1_DBG, 0},
    {"SC2_FRM",       MCM_ID_SC2_FRM, 0},
    {"SC3_FRM",       MCM_ID_SC3_FRM, 0},
    {"FCIE",          MCM_ID_FCIE, 0},
    {"SDIO",          MCM_ID_SDIO, 0},
    {"SC1_SNPI",      MCM_ID_SC1_SNPI, 0},
    {"SC2_SNPI",      MCM_ID_SC2_SNPI, 0},
    {"*ALL_CLIENTS*", MCM_ID_ALL, 0}  //use carefully
};

struct device mcm_dev;

static struct bus_type mcm_subsys = {
    .name = "mcm",
    .dev_name = "mcm",
};

static int mcm_rw(int index, int ratio, int write)
{
    int i, addr;

    if(index == MCM_ID_ALL && write)
    {
        for(i=0; i<(sizeof(mcm_clients)/sizeof(mcm_clients[0]))-1;i++)
            mcm_rw(i, ratio, write);
        return 0;
    }
    else if(index == MCM_ID_MCU51)
        addr = BASE_REG_MCM_DIG_GP_PA + 0x0;
    else if (index == MCM_ID_URDMA)
        addr = BASE_REG_MCM_DIG_GP_PA + 0x1;
    else if (index == MCM_ID_BDMA)
        addr = BASE_REG_MCM_DIG_GP_PA + 0x4;
    else if (index == MCM_ID_VHE)
        addr = BASE_REG_MCM_VHE_GP_PA + 0x0;
    else if (index == MCM_ID_MFE)
        addr = BASE_REG_MCM_SC_GP_PA + 0x0;
    else if (index == MCM_ID_JPE)
        addr = BASE_REG_MCM_SC_GP_PA + 0x1;
    else if (index == MCM_ID_BACH)
        addr = BASE_REG_MCM_SC_GP_PA + 0x4;
    else if (index == MCM_ID_AESDMA)
        addr = BASE_REG_MCM_SC_GP_PA + 0x5;
    else if (index == MCM_ID_UHC)
        addr = BASE_REG_MCM_SC_GP_PA + 0x8;
    else if (index == MCM_ID_EMAC)
        addr = BASE_REG_MCM_SC_GP_PA + 0x9;
    else if (index == MCM_ID_CMDQ)
        addr = BASE_REG_MCM_SC_GP_PA + 0xC;
    else if (index == MCM_ID_ISP_DNR)
        addr = BASE_REG_MCM_SC_GP_PA + 0xD;
    else if (index == MCM_ID_ISP_DMA)
        addr = BASE_REG_MCM_SC_GP_PA + 0x10;
    else if (index == MCM_ID_GOP)
        addr = BASE_REG_MCM_SC_GP_PA + 0x11;
    else if (index == MCM_ID_SC_DNR)
        addr = BASE_REG_MCM_SC_GP_PA + 0x14;
    else if (index == MCM_ID_SC_DNR_SAD)
        addr = BASE_REG_MCM_SC_GP_PA + 0x15;
    else if (index == MCM_ID_SC_CROP)
        addr = BASE_REG_MCM_SC_GP_PA + 0x18;
    else if (index == MCM_ID_SC1_FRM)
        addr = BASE_REG_MCM_SC_GP_PA + 0x19;
    else if (index == MCM_ID_SC1_SNP)
        addr = BASE_REG_MCM_SC_GP_PA + 0x1C;
    else if (index == MCM_ID_SC1_DBG)
        addr = BASE_REG_MCM_SC_GP_PA + 0x1D;
    else if (index == MCM_ID_SC2_FRM)
        addr = BASE_REG_MCM_SC_GP_PA + 0x20;
    else if (index == MCM_ID_SC3_FRM)
        addr = BASE_REG_MCM_SC_GP_PA + 0x21;
    else if (index == MCM_ID_FCIE)
        addr = BASE_REG_MCM_SC_GP_PA + 0x24;
    else if (index == MCM_ID_SDIO)
        addr = BASE_REG_MCM_SC_GP_PA + 0x25;
    else if (index == MCM_ID_SC1_SNPI)
        addr = BASE_REG_MCM_SC_GP_PA + 0x28;
    else if (index == MCM_ID_SC2_SNPI)
        addr = BASE_REG_MCM_SC_GP_PA + 0x29;
    else
    {
        printk(KERN_ERR "mcm_clients[%d] not exists\n", index);
        return -1;
    }

    if(write)
        OUTREG8(addr, (ratio << 4));
    else
        mcm_clients[index].slow_down_ratio = (INREG8(addr) >> 4);

    return 0;
}


static ssize_t mcm_slow_ratio_store(struct device *dev,  struct device_attribute *attr, const char *buf, size_t n)
{
    int index, ratio;
    sscanf(buf, "%d %d", &index, &ratio);

    if(0 > ratio || ratio > 15)
    {
        printk(KERN_ERR "MCM slow down ratio should be 0~15\n");
        return -1;
    }

    return mcm_rw(index, ratio, 1)?-1:n;
}

static ssize_t mcm_slow_ratio_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;
    int i=0;

    for(i=0; i<(sizeof(mcm_clients)/sizeof(mcm_clients[0]))-1;i++)
    {
        mcm_rw(i, 0, 0);
        str += scnprintf(str, end - str, "[%d] %s: %d\n", mcm_clients[i].index, mcm_clients[i].name, mcm_clients[i].slow_down_ratio);
    }
    str += scnprintf(str, end - str, "[%d] %s\n", mcm_clients[i].index, mcm_clients[i].name);

    return (str - buf);
}

DEVICE_ATTR(mcm_slow_ratio, 0644, mcm_slow_ratio_show, mcm_slow_ratio_store);

static void __init infinity_create_MCM_node(void)
{
    int ret;

    mcm_dev.kobj.name="mcm";
    mcm_dev.bus=&mcm_subsys;

    ret = subsys_system_register(&mcm_subsys, NULL);
    if (ret)
    {
        printk(KERN_ERR "Failed to register MCM sub system!! %d\n",ret);
        return;
    }

    ret = device_register(&mcm_dev);
    if(ret)
    {
        printk(KERN_ERR "Failed to register MCM device!! %d\n",ret);
        return;
    }

    device_create_file(&mcm_dev, &dev_attr_mcm_slow_ratio);
}





extern int infinity_pm_init(void);
extern void init_proc_zen(void);
static inline void __init infinity_init_late(void)
{
    infinity_pm_init();
    infinity_create_MIU_node();
    infinity_create_MCM_node();
}

DT_MACHINE_START(MS_INFINITY_DT, "MStar Infinity (Flattened Device Tree)")
    .dt_compat    = infinity_dt_compat,
    .map_io = infinity_map_io,
    .init_machine = infinity_init_machine,
    .init_early = infinity_init_early,
//    .init_time =  ms_init_timer,
//    .init_irq = infinity_init_irqchip,
    .init_late = infinity_init_late,
    .restart = infinity_restart,  //in reset.S
MACHINE_END
