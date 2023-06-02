#include <linux/pfn.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>          /* seems do not need this */
#include <linux/delay.h>
#include <linux/interrupt.h>

#include <linux/module.h>
#include <linux/kernel.h>
#include <asm/uaccess.h>
#include <linux/fs.h>
#include <asm/io.h>
#include <asm/string.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/poll.h>

#include "irqs.h"
#include "ms_platform.h"
#include "ms_msys.h"

#include "mdrv_dip.h"
#include "mdrv_dip_io_st.h"
#include "mdrv_dip_io.h"
#include "MsTypes.h"
#include "drvXC_IOPort.h"
#include "apiXC.h"
#include "drv_sc_isr.h"

#define MDRV_DIP_DEVICE_COUNT   1
#define MDRV_DIP_NAME           "mdip"
#define MAX_FILE_HANDLE_SUPPRT  64
#define MDRV_NAME_DIP           "mdip"
#define MDRV_MAJOR_DIP          0xea
#define MDRV_MINOR_DIP          0x02


#define CMD_PARSING(x)  (x==IOCTL_DIP_SET_INTERRUPT ?         "IOCTL_DIP_SET_INTERRUPT" : \
                         x==IOCTL_DIP_SET_CONFIG ?            "IOCTL_DIP_SET_CONFIG" : \
                         x==IOCTL_DIP_SET_WONCE_BASE_CONFIG ? "IOCTL_DIP_SET_WONCE_BASE_CONFIG" : \
                         x==IOCTL_DIP_GET_INTERRUPT_STATUS ?  "IOCTL_DIP_GET_INTERRUPT_STATUS" : \
                                                            "unknown")


int mstar_dip_drv_open(struct inode *inode, struct file *filp);
int mstar_dip_drv_release(struct inode *inode, struct file *filp);
long mstar_dip_drv_ioctl(struct file *filp, unsigned int u32Cmd, unsigned long u32Arg);
static int mstar_dip_drv_probe(struct platform_device *pdev);
static int mstar_dip_drv_remove(struct platform_device *pdev);
static int mstar_dip_drv_suspend(struct platform_device *dev, pm_message_t state);
static int mstar_dip_drv_resume(struct platform_device *dev);
static void mstar_dip_drv_platfrom_release(struct device *device);
static unsigned int mstar_dipw_drv_poll(struct file *filp, struct poll_table_struct *wait);



extern atomic_t DIPW_intr_count;

//-------------------------------------------------------------------------------------------------

typedef struct
{
    int s32Major;
    int s32Minor;
    int refCnt;
    struct cdev cdev;
    struct file_operations fops;
}DIP_DEV;

static DIP_DEV _devDIP =
{
    .s32Major = MDRV_MAJOR_DIP,
    .s32Minor = MDRV_MINOR_DIP,
    .refCnt = 0,
    .cdev =
    {
        .kobj = {.name= MDRV_NAME_DIP, },
        .owner = THIS_MODULE,
    },
    .fops =
    {
        .open = mstar_dip_drv_open,
        .release = mstar_dip_drv_release,
        .unlocked_ioctl = mstar_dip_drv_ioctl,
        .poll = mstar_dipw_drv_poll,
    }
};

static struct class * dip_class;
static char * dip_classname = "mstar_dip_class";

static struct platform_driver Mstar_dip_driver = {
	.probe 		= mstar_dip_drv_probe,
	.remove 	= mstar_dip_drv_remove,
    .suspend    = mstar_dip_drv_suspend,
    .resume     = mstar_dip_drv_resume,

	.driver = {
		.name	= "mdip",
        .owner  = THIS_MODULE,
	}
};

static u64 sg_mstar_device_dip_dmamask = 0xffffffffUL;

static struct platform_device sg_mdrv_dip_device =
{
    .name = "mdip",
    .id = 0,
    .dev =
    {
        .release = mstar_dip_drv_platfrom_release,
        .dma_mask = &sg_mstar_device_dip_dmamask,
        .coherent_dma_mask = 0xffffffffUL
    }
};
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
// IOCtrl Driver interface functions
//-------------------------------------------------------------------------------------------------


//----------------------------------------------------------------------------------------------



MS_BOOL DIP_Set_Config(PDIP_CONFIG pCfg)
{
    MS_BOOL bret = TRUE;
    XC_DIP_CONFIG_t stDipConfig;

    if(pCfg->bTriggle == FALSE)
    {

        if(pCfg->enTrigMode == DIP_TRIGGER_LOOP)
        {
            MApi_XC_Set_Dip_Trig(FALSE);
        }
        else
        {
            MApi_XC_DIP_Set_Dwin_Wt_Once(FALSE);
        }
    }
    else
    {

        memset(&stDipConfig, 0, sizeof(XC_DIP_CONFIG_t));

        // clip window
        stDipConfig.bClipEn = pCfg->bClipEn;
        stDipConfig.stClipWin.x = pCfg->stClipWin.x < 1 ? 1 : pCfg->stClipWin.x;
        stDipConfig.stClipWin.y = pCfg->stClipWin.y < 1 ? 1 : pCfg->stClipWin.y;
        stDipConfig.stClipWin.width = pCfg->stClipWin.width > pCfg->u16Width ? pCfg->u16Width : pCfg->stClipWin.width;
        stDipConfig.stClipWin.height = pCfg->stClipWin.height > pCfg->u16Height ? pCfg->u16Height : pCfg->stClipWin.height;


        // pitch
        stDipConfig.u16Pitch = pCfg->u16Width;

        // width/height
        stDipConfig.u16Width = pCfg->u16Width;
        stDipConfig.u16Height = pCfg->u16Height;

        // frame number
        stDipConfig.u8FrameNum = pCfg->u8FrameNum;

        // base address & offset
        stDipConfig.u32BaseAddr = pCfg->u32BuffAddress;
        stDipConfig.u32BaseSize = pCfg->u32BuffSize;

        // C base address & offset
        if(pCfg->enDestFmtType == DIP_DEST_FMT_YC420_MVOP ||
           pCfg->enDestFmtType == DIP_DEST_FMT_YC420_MFE )
        {
            stDipConfig.u32C_BaseAddr = pCfg->u32C_BuffAddress;
            stDipConfig.u32C_BaseSize = pCfg->u32C_BuffSize;
            stDipConfig.bSrc422 = TRUE;
            stDipConfig.b444To422 = TRUE;

        }
        else if(DIP_DEST_FMT_YC422==pCfg->enDestFmtType)
        {
            stDipConfig.bSrc422   = TRUE;
            stDipConfig.b444To422 = TRUE;
        }
        else
        {
            stDipConfig.bSrc422 = FALSE;
            stDipConfig.b444To422 = FALSE;
        }

        stDipConfig.eDstFMT = pCfg->enDestFmtType == DIP_DEST_FMT_YC422      ? DIP_DST_FMT_YC422 :
                              pCfg->enDestFmtType == DIP_DEST_FMT_ARGB8888   ? DIP_DST_FMT_ARGB8888:
                              pCfg->enDestFmtType == DIP_DEST_FMT_RGB565     ? DIP_DST_FMT_RGB565:
                              pCfg->enDestFmtType == DIP_DEST_FMT_YC420_MVOP ? DIP_DST_FMT_YC420_MVOP :
                                                                                   DIP_DST_FMT_YC420_MFE;
        stDipConfig.eSrcFMT = DIP_SRC_FMT_OP;
        stDipConfig.bSrcYUV = TRUE;
        stDipConfig.bWOnce = pCfg->enTrigMode == DIP_TRIGGER_ONCE ? TRUE : FALSE;

        DIP_DBG(DIP_DBG_LV_0, "SetDip: clip(%d, %d, %d, %d), w=%d, h:%d, FBnum:%d, Base:%x, Size:%x, CBase:%x, CSize:%x\n",
            stDipConfig.stClipWin.x, stDipConfig.stClipWin.y, stDipConfig.stClipWin.width, stDipConfig.stClipWin.height,
            stDipConfig.u16Width, stDipConfig.u16Height, stDipConfig.u8FrameNum,
            (int)stDipConfig.u32BaseAddr, (int)stDipConfig.u32BaseSize,
            (int)stDipConfig.u32C_BaseAddr, (int)stDipConfig.u32C_BaseSize);

        if(MApi_XC_Set_Dip_Config(stDipConfig) == E_APIXC_RET_FAIL)
        {
            bret = FALSE;
        }
        else
        {
            if(pCfg->enTrigMode == DIP_TRIGGER_LOOP)
            {
                MApi_XC_Set_Dip_Trig(TRUE);
            }
            else
            {
                DIP_DBG(DIP_DBG_LV_0, "Wonce On \n");
                MApi_XC_DIP_Set_Dwin_Wt_Once(TRUE);
                MApi_XC_DIP_Set_Dwin_Wt_Once_Trig(TRUE);
            }
        }
    }

    return bret;
}


MS_BOOL Dip_Set_WOnce_Base(PDIP_WONCE_BASE_CONFIG pCfg)
{
    MS_BOOL bret = TRUE;

    if(pCfg->bTrig)
    {
        if(MApi_XC_DIP_Set_Base(pCfg->u32BuffAddress, pCfg->u32C_BuffAddress))
        {
            MApi_XC_DIP_Set_Dwin_Wt_Once_Trig(TRUE);
        }
        else
        {
            bret = FALSE;
        }
    }

    return bret;
}



int _MDrv_DIPIO_IOC_Set_Interrupt(struct file *filp, unsigned long arg)
{
	DIP_INTERRUPT_CONFIG stCfg;
    if(copy_from_user(&stCfg, (DIP_INTERRUPT_CONFIG __user *)arg, sizeof(DIP_INTERRUPT_CONFIG)))
    {
    	printk("SCA FAIL %d ...\n",__LINE__);
        return -EFAULT;
    }


    if(stCfg.bEn  == true && MDrv_DIPW_InterruptIsAttached(0, NULL, NULL) == false )
    {
        MDrv_DIPW_InterruptAttach(0, NULL, NULL);
    }
    else if(stCfg.bEn == false && MDrv_DIPW_InterruptIsAttached(0, NULL ,NULL) == true )
    {
        MDrv_DIPW_InterruptDeAttach(0, NULL, NULL);
    }

    if(stCfg.u32DIPW_Signal_PID)
    {
        MDrv_DIPW_Set_Signal_PID(stCfg.u32DIPW_Signal_PID);
    }

    return 0;
}

int _MDrv_DIPIO_IOC_Set_Dip_Config(struct file *filp, unsigned long arg)
{
    int ret  = 0;
    DIP_CONFIG stCfg;

    if(copy_from_user(&stCfg, (DIP_CONFIG __user *)arg, sizeof(DIP_CONFIG)))
    {
        return -EFAULT;
    }

    if(!DIP_Set_Config(&stCfg))
    {
        ret = -EFAULT;
    }
    return ret;
}


int _MDrv_DIPIO_IOC_Set_Dip_WOnce_Base(struct file *filp, unsigned long arg)
{
    int ret  = 0;
    DIP_WONCE_BASE_CONFIG stCfg;

    if(copy_from_user(&stCfg, (DIP_WONCE_BASE_CONFIG __user *)arg, sizeof(DIP_WONCE_BASE_CONFIG)))
    {
        return -EFAULT;
    }

    if(!Dip_Set_WOnce_Base(&stCfg))
    {
        ret = -EFAULT;
    }

    return ret;
}

int _MDrv_DIPIO_IO_Get_Interrupt_Status(struct file *filp, unsigned long arg)
{
    DIP_INTR_STATUS stIntrStatus;

    memset(&stIntrStatus, 0, sizeof(DIP_INTR_STATUS));
    stIntrStatus.u8DIPW_Status = MDrv_DIPW_Get_ISR_RecFlag();

     if(copy_to_user((DIP_INTR_STATUS __user *)arg, &stIntrStatus, sizeof(DIP_INTR_STATUS)))
    {
    	printk("SCA FAIL : %d \n",__LINE__);
        return -EFAULT;
    }
    return 0;
}

//==============================================================================
long mstar_dip_drv_ioctl(struct file *filp, unsigned int u32Cmd, unsigned long u32Arg)
{
    int err = 0;
    int retval = 0;

    if(_devDIP.refCnt <= 0)
    {
        DIP_DBG(DIP_DBG_LV_IOCTL, "[DIP] DIPIO_IOCTL refCnt =%d!!! \n", _devDIP.refCnt);
        return -EFAULT;
    }
    /* check u32Cmd valid */
    if(IOCTL_DIP_MAGIC == _IOC_TYPE(u32Cmd))
    {
        if(_IOC_NR(u32Cmd) >= IOCTL_DIP_MAX_NR)
        {
            DIP_DBG(DIP_DBG_LV_0, "[DIP] IOCtl NR Error!!! (Cmd=%x)\n",u32Cmd);
            return -ENOTTY;
        }
    }
    else
    {
        DIP_DBG(DIP_DBG_LV_0, "[DIP] IOCtl MAGIC Error!!! (Cmd=%x)\n",u32Cmd);
        return -ENOTTY;
    }

    /* verify Access */
    if (_IOC_DIR(u32Cmd) & _IOC_READ)
    {
        err = !access_ok(VERIFY_WRITE, (void __user *)u32Arg, _IOC_SIZE(u32Cmd));
    }
    else if (_IOC_DIR(u32Cmd) & _IOC_WRITE)
    {
        err =  !access_ok(VERIFY_READ, (void __user *)u32Arg, _IOC_SIZE(u32Cmd));
    }
    if (err)
    {
        return -EFAULT;
    }
	/* not allow query or command once driver suspend */

    DIP_DBG(DIP_DBG_LV_IOCTL, "[SCA] SCAIO_IOCTL %s!!! \n", (CMD_PARSING(u32Cmd)));

    switch(u32Cmd)
    {
    case IOCTL_DIP_SET_INTERRUPT:
        retval = _MDrv_DIPIO_IOC_Set_Interrupt(filp, u32Arg);
        break;

    case IOCTL_DIP_SET_CONFIG:
        retval = _MDrv_DIPIO_IOC_Set_Dip_Config(filp, u32Arg);
        break;

    case IOCTL_DIP_SET_WONCE_BASE_CONFIG:
        retval = _MDrv_DIPIO_IOC_Set_Dip_WOnce_Base(filp, u32Arg);
        break;

    case IOCTL_DIP_GET_INTERRUPT_STATUS:
        retval = _MDrv_DIPIO_IO_Get_Interrupt_Status(filp, u32Arg);
        break;

    default:  /* redundant, as cmd was checked against MAXNR */
        DIP_DBG(DIP_DBG_LV_0, "[DIP] ERROR IOCtl number %x\n ",u32Cmd);
        retval = -ENOTTY;
        break;
    }

    return retval;
}


static unsigned int mstar_dipw_drv_poll(struct file *filp, struct poll_table_struct *wait)
{
    if (atomic_read(&DIPW_intr_count) > 0)
    {
        atomic_set(&DIPW_intr_count,0);
        return POLLIN | POLLRDNORM; /* readable */
    }

    return 0;
}

static int mstar_dip_drv_probe(struct platform_device *pdev)
{
    DIP_DBG(DIP_DBG_LV_IOCTL, "[DIP] mstar_dip_drv_probe\n");


	if(request_irq(INT_IRQ_DIPW, MDrv_DIPW_isr, 0, "DIPW_INTR ", NULL))
		   DIP_DBG(DIP_DBG_LV_IOCTL, "[DIP] request irq for dipw fail!\n");

    return 0;
}

static int mstar_dip_drv_remove(struct platform_device *pdev)
{
    DIP_DBG(DIP_DBG_LV_IOCTL, "[DIP] mstar_dip_drv_remove\n");

    return 0;
}


static int mstar_dip_drv_suspend(struct platform_device *dev, pm_message_t state)
{
    DIP_DBG(DIP_DBG_LV_IOCTL, "[DIP] mstar_dip_drv_suspend\n");


    return 0;
}


static int mstar_dip_drv_resume(struct platform_device *dev)
{
    DIP_DBG(DIP_DBG_LV_IOCTL, "[DIP] mstar_dip_drv_resume\n");


    return 0;
}

static void mstar_dip_drv_platfrom_release(struct device *device)
{
    DIP_DBG(DIP_DBG_LV_IOCTL, "[DIP] mstar_dip_drv_platfrom_release\n");

}


int mstar_dip_drv_open(struct inode *inode, struct file *filp)
{
    DIP_DBG(DIP_DBG_LV_IOCTL, "[DIP] DIP DRIVER OPEN\n");

    DIPIO_ASSERT(_devDIP.refCnt>=0);
    _devDIP.refCnt++;



    return 0;
}


int mstar_dip_drv_release(struct inode *inode, struct file *filp)
{

    DIP_DBG(DIP_DBG_LV_IOCTL, "[DIP] DIP DRIVER RELEASE\n");
    _devDIP.refCnt--;
    DIPIO_ASSERT(_devDIP.refCnt>=0);

    free_irq(INT_IRQ_DIPW, MDrv_DIPW_isr);

    return 0;
}


//-------------------------------------------------------------------------------------------------
// Module functions
//-------------------------------------------------------------------------------------------------
int _MDrv_DIPIO_ModuleInit(void)
{
    int ret = 0;
    dev_t  dev;
    int s32Ret;
    DIP_DBG(DIP_DBG_LV_IOCTL, "[DIPIO]_Init \n");

    if(_devDIP.s32Major)
    {
        dev = MKDEV(_devDIP.s32Major, _devDIP.s32Minor);
        s32Ret = register_chrdev_region(dev, MDRV_DIP_DEVICE_COUNT, MDRV_DIP_NAME);
    }
    else
    {
        s32Ret = alloc_chrdev_region(&dev, _devDIP.s32Minor, MDRV_DIP_DEVICE_COUNT, MDRV_DIP_NAME);
        _devDIP.s32Major = MAJOR(dev);
    }

    if (0 > s32Ret)
    {
        DIP_DBG(DIP_DBG_LV_0, "[DIP] Unable to get major %d\n", _devDIP.s32Major);
        return s32Ret;
    }

    cdev_init(&_devDIP.cdev, &_devDIP.fops);
    if (0 != (s32Ret= cdev_add(&_devDIP.cdev, dev, MDRV_DIP_DEVICE_COUNT)))
    {
        DIP_DBG(DIP_DBG_LV_0, "[DIP] Unable add a character device\n");
        unregister_chrdev_region(dev, MDRV_DIP_DEVICE_COUNT);
        return s32Ret;
    }

    dip_class = class_create(THIS_MODULE, dip_classname);
    if(IS_ERR(dip_class))
    {
        printk(KERN_WARNING"Failed at class_create().Please exec [mknod] before operate the device/n");
    }
    else
    {
        device_create(dip_class, NULL, dev,NULL, "mdip");
    }
    /* initial the whole DIP Driver */
    ret = platform_driver_register(&Mstar_dip_driver);

    if (!ret)
    {
        ret = platform_device_register(&sg_mdrv_dip_device);
        if (ret)    /* if register device fail, then unregister the driver.*/
        {
            platform_driver_unregister(&Mstar_dip_driver);
            DIP_DBG(DIP_DBG_LV_0, "[DIP] register failed\n");

        }
        else
        {
            DIP_DBG(DIP_DBG_LV_IOCTL, "[DIP] register success\n");
        }
    }


    return ret;
}


void _MDrv_DIPIO_ModuleExit(void)
{
    /*de-initial the who GFLIPDriver */
    DIP_DBG(DIP_DBG_LV_0, "[DIPIO]_Exit \n");
    cdev_del(&_devDIP.cdev);
    device_destroy(dip_class, MKDEV(_devDIP.s32Major, _devDIP.s32Minor));
    class_destroy(dip_class);
    unregister_chrdev_region(MKDEV(_devDIP.s32Major, _devDIP.s32Minor), MDRV_DIP_DEVICE_COUNT);
    platform_driver_unregister(&Mstar_dip_driver);
}


module_init(_MDrv_DIPIO_ModuleInit);
module_exit(_MDrv_DIPIO_ModuleExit);

MODULE_AUTHOR("MSTAR");
MODULE_DESCRIPTION("DIPioctrl driver");
MODULE_LICENSE("DIP");


