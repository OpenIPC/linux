#include <linux/pfn.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/mm.h>               /* for dma_alloc_coherent */
#include <linux/slab.h>
#include <linux/vmalloc.h>          /* seems do not need this */
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>      /* for dma_alloc_coherent */

#include <linux/module.h>
#include <linux/kernel.h>
#include <asm/uaccess.h>
#include <linux/fs.h>
#include <asm/io.h>
#include <asm/string.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>

#include "mdrv_gop.h"
#include "mdrv_gop_io.h"

#define MDRV_GOP_DEVICE_COUNT   1
#define MDRV_GOP_NAME           "mstar_gop"
#define MAX_FILE_HANDLE_SUPPRT  64
#define MDRV_NAME_GOP           "mstar_gop"
#define MDRV_MAJOR_GOP          0xea
#define MDRV_MINOR_GOP          0x01


int mstar_gop_drv_open(struct inode *inode, struct file *filp);
int mstar_gop_drv_release(struct inode *inode, struct file *filp);
long mstar_gop_drv_ioctl(struct file *filp, unsigned int u32Cmd, unsigned long u32Arg);
static int mstar_gop_drv_probe(struct platform_device *pdev);
static int mstar_gop_drv_remove(struct platform_device *pdev);
static int mstar_gop_drv_suspend(struct platform_device *dev, pm_message_t state);
static int mstar_gop_drv_resume(struct platform_device *dev);
static void mstar_gop_drv_platfrom_release(struct device *device);
#if defined (CONFIG_ARCH_CEDRIC)
extern int mstar_sca_drv_get_h_backporch(int sc_num);
#endif
//-------------------------------------------------------------------------------------------------

typedef struct
{
    int s32Major;
    int s32Minor;
    int refCnt;
    struct cdev cdev;
    struct file_operations fops;
}GOP_DEV;

static GOP_DEV _devGOP =
{
    .s32Major = MDRV_MAJOR_GOP,
    .s32Minor = MDRV_MINOR_GOP,
    .refCnt = 0,
    .cdev =
    {
        .kobj = {.name= MDRV_NAME_GOP, },
        .owner = THIS_MODULE,
    },
    .fops =
    {
        .open = mstar_gop_drv_open,
        .release = mstar_gop_drv_release,
        .unlocked_ioctl = mstar_gop_drv_ioctl,
    }
};

static struct class * gop_class;
static char * gop_classname = "mstar_gop_class";


static struct platform_driver Mstar_gop_driver = {
	.probe 		= mstar_gop_drv_probe,
	.remove 	= mstar_gop_drv_remove,
    .suspend    = mstar_gop_drv_suspend,
    .resume     = mstar_gop_drv_resume,

	.driver = {
		.name	= "mstar_gop",
        .owner  = THIS_MODULE,
	}
};

static u64 sg_mstar_device_gop_dmamask = 0xffffffffUL;

static struct platform_device sg_mdrv_gop_device =
{
    .name = "mstar_gop",
    .id = 0,
    .dev =
    {
        .release = mstar_gop_drv_platfrom_release,
        .dma_mask = &sg_mstar_device_gop_dmamask,
        .coherent_dma_mask = 0xffffffffUL
    }
};
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
// IOCtrl Driver interface functions
//-------------------------------------------------------------------------------------------------
int _MDrv_GOPIO_IOC_Set_BaseAddr(struct file *filp, unsigned long arg)
{
    GOP_SET_BASE_ADDR_CONFIG stCfg;

    if(copy_from_user(&stCfg, (GOP_SET_BASE_ADDR_CONFIG __user *)arg, sizeof(GOP_SET_BASE_ADDR_CONFIG)))
    {
        return -EFAULT;
    }

    GOP_DBG(GOP_DBG_LV_0, "SetBase: GOP_%d, Base=%x, \n", stCfg.enGopNum, (int)stCfg.u32Base);

    GOP_Pan_Display(stCfg.enGopNum, stCfg.u32Base);


    return 0;
}

int _MDrv_GOPIO_IOC_SetMode(struct file *filp, unsigned long arg)
{
    GOP_SETMODE_CONFIG stCfg;
    int Pixel_Format, Bytes_Per_Pixel;
    if(copy_from_user(&stCfg, (GOP_SETMODE_CONFIG __user *)arg, sizeof(GOP_SETMODE_CONFIG)))
    {
        return -EFAULT;
    }

    GOP_DBG(GOP_DBG_LV_0, "SetMode: GOP_%d, (%d, %d) FM=%d, Base=%x, interlace=%d, yuv=%d,\n",
        stCfg.enGopNum, stCfg.u16width, stCfg.u16height, stCfg.enSrcFormat, (int)stCfg.u32BaseAddr, stCfg.bInterlace, stCfg.bYUVOutput);

    switch(stCfg.enSrcFormat)
    {
    case E_GOP_SRC_RGB565:
        Pixel_Format = 0x0010;
        Bytes_Per_Pixel = 2;
        break;

    case E_GOP_SRC_ARGB8888:
        Pixel_Format = 0x0050;
        Bytes_Per_Pixel = 4;
        break;

    case E_GOP_SRC_ABGR8888:
        Pixel_Format = 0x0070;
        Bytes_Per_Pixel = 4;
        break;

    }


    GOP_Setmode(FALSE, stCfg.enGopNum, stCfg.u16width, stCfg.u16height, stCfg.u32BaseAddr, Pixel_Format, Bytes_Per_Pixel, stCfg.bInterlace);

    GOP_Set_OutFormat(stCfg.enGopNum, stCfg.bYUVOutput);
#if defined (CONFIG_ARCH_CEDRIC)
    if(stCfg.enGopNum == E_GOP_1)
    {
        GOP_Set_PipeDelay(1,  mstar_sca_drv_get_h_backporch(1) );
    }
#endif
    return 0;
}


int _MDrv_GOPIO_IOC_Set_Constant_Alpha(struct file *filp, unsigned long arg)
{
    GOP_SET_CONSTANT_ALPHA_CONFIG stCfg;
    if(copy_from_user(&stCfg, (GOP_SET_CONSTANT_ALPHA_CONFIG __user *)arg, sizeof(GOP_SET_CONSTANT_ALPHA_CONFIG)))
    {
        return -EFAULT;
    }

    GOP_Set_Constant_Alpha(stCfg.enGopNum, stCfg.bEn, stCfg.u8Alpha);

    return 0;
}

int _MDrv_GOPIO_IOC_Set_Color_Key(struct file *filp, unsigned long arg)
{
    GOP_SET_COLOR_KEY_CONFIG stCfg;
    if(copy_from_user(&stCfg, (GOP_SET_COLOR_KEY_CONFIG __user *)arg, sizeof(GOP_SET_COLOR_KEY_CONFIG)))
    {
        return -EFAULT;
    }

    GOP_Set_Color_Key(stCfg.enGopNum, stCfg.bEn, stCfg.u8R, stCfg.u8G, stCfg.u8B);

    return 0;
}

//==============================================================================
long mstar_gop_drv_ioctl(struct file *filp, unsigned int u32Cmd, unsigned long u32Arg)
{
    int err = 0;
    int retval = 0;

    if(_devGOP.refCnt <= 0)
    {
        GOP_DBG(GOP_DBG_LV_IOCTL, "[GOP] GOPIO_IOCTL refCnt =%d!!! \n", _devGOP.refCnt);
        return -EFAULT;
    }
    /* check u32Cmd valid */
    if(GOP_IOC_MAGIC == _IOC_TYPE(u32Cmd))
    {
        if(_IOC_NR(u32Cmd) >= GOP_IOC_MAX_NR)
        {
            GOP_DBG(GOP_DBG_LV_0, "[GOP] IOCtl NR Error!!! (Cmd=%x)\n",u32Cmd);
            return -ENOTTY;
        }
    }
    else
    {
        GOP_DBG(GOP_DBG_LV_0, "[GOP] IOCtl MAGIC Error!!! (Cmd=%x)\n",u32Cmd);
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

    switch(u32Cmd)
    {
    case GOP_IOC_SET_BASE_ADDR:
        retval = _MDrv_GOPIO_IOC_Set_BaseAddr(filp, u32Arg);
        break;

    case GOP_IOC_SET_MODE:
        retval = _MDrv_GOPIO_IOC_SetMode(filp, u32Arg);
        break;

    case GOP_IOC_SET_CONSTANT_ALPHA:
        retval = _MDrv_GOPIO_IOC_Set_Constant_Alpha(filp, u32Arg);
        break;

    case GOP_IOC_SET_COLOR_KEY:
       retval = _MDrv_GOPIO_IOC_Set_Color_Key(filp, u32Arg);
        break;

    default:  /* redundant, as cmd was checked against MAXNR */
        GOP_DBG(GOP_DBG_LV_0, "[GOP] ERROR IOCtl number %x\n ",u32Cmd);
        retval = -ENOTTY;
        break;
    }

    return retval;
}

static int mstar_gop_drv_probe(struct platform_device *pdev)
{
    GOP_DBG(GOP_DBG_LV_IOCTL, "[GOP] mstar_gop_drv_probe\n");

    return 0;
}

static int mstar_gop_drv_remove(struct platform_device *pdev)
{
    GOP_DBG(GOP_DBG_LV_IOCTL, "[GOP] mstar_gop_drv_remove\n");

    return 0;
}


static int mstar_gop_drv_suspend(struct platform_device *dev, pm_message_t state)
{
    GOP_DBG(GOP_DBG_LV_IOCTL, "[GOP] mstar_gop_drv_suspend\n");


    return 0;
}


static int mstar_gop_drv_resume(struct platform_device *dev)
{
    GOP_DBG(GOP_DBG_LV_IOCTL, "[GOP] mstar_gop_drv_resume\n");


    return 0;
}

static void mstar_gop_drv_platfrom_release(struct device *device)
{
    GOP_DBG(GOP_DBG_LV_IOCTL, "[GOP] mstar_gop_drv_platfrom_release\n");

}


int mstar_gop_drv_open(struct inode *inode, struct file *filp)
{
    GOP_DBG(GOP_DBG_LV_IOCTL, "[GOP] GOP DRIVER OPEN\n");

    GOPIO_ASSERT(_devGOP.refCnt>=0);
    _devGOP.refCnt++;



    return 0;
}


int mstar_gop_drv_release(struct inode *inode, struct file *filp)
{

    GOP_DBG(GOP_DBG_LV_IOCTL, "[GOP] GOP DRIVER RELEASE\n");
    _devGOP.refCnt--;
    GOPIO_ASSERT(_devGOP.refCnt>=0);

    return 0;
}


//-------------------------------------------------------------------------------------------------
// Module functions
//-------------------------------------------------------------------------------------------------
int _MDrv_GOPIO_ModuleInit(void)
{
    int s32Ret;
    dev_t  dev;
    int ret = 0;

    GOP_DBG(GOP_DBG_LV_IOCTL, "[GOPIO]_Init \n");

    if(_devGOP.s32Major)
    {
        dev = MKDEV(_devGOP.s32Major, _devGOP.s32Minor);
        s32Ret = register_chrdev_region(dev, MDRV_GOP_DEVICE_COUNT, MDRV_GOP_NAME);
    }
    else
    {
        s32Ret = alloc_chrdev_region(&dev, _devGOP.s32Minor, MDRV_GOP_DEVICE_COUNT, MDRV_GOP_NAME);
        _devGOP.s32Major = MAJOR(dev);
    }

    if (0 > s32Ret)
    {
        GOP_DBG(GOP_DBG_LV_0, "[GOP] Unable to get major %d\n", _devGOP.s32Major);
        return s32Ret;
    }

    cdev_init(&_devGOP.cdev, &_devGOP.fops);
    if (0 != (s32Ret= cdev_add(&_devGOP.cdev, dev, MDRV_GOP_DEVICE_COUNT)))
    {
        GOP_DBG(GOP_DBG_LV_0, "[GOP] Unable add a character device\n");
        unregister_chrdev_region(dev, MDRV_GOP_DEVICE_COUNT);
        return s32Ret;
    }

    gop_class = class_create(THIS_MODULE, gop_classname);
    if(IS_ERR(gop_class))
    {
        printk(KERN_WARNING"Failed at class_create().Please exec [mknod] before operate the device/n");
    }
    else
    {
        device_create(gop_class, NULL, dev,NULL, "mstar_gop");
    }

    /* initial the whole GOP Driver */
    ret = platform_driver_register(&Mstar_gop_driver);

    if (!ret)
    {
        ret = platform_device_register(&sg_mdrv_gop_device);
        if (ret)    /* if register device fail, then unregister the driver.*/
        {
            platform_driver_unregister(&Mstar_gop_driver);
            GOP_DBG(GOP_DBG_LV_0, "[GOP] register failed\n");

        }
        else
        {
            GOP_DBG(GOP_DBG_LV_IOCTL, "[GOP] register success\n");
        }
    }


    return ret;
}


void _MDrv_GOPIO_ModuleExit(void)
{
    /*de-initial the who GFLIPDriver */
    GOP_DBG(GOP_DBG_LV_0, "[GOPIO]_Exit \n");

    cdev_del(&_devGOP.cdev);
    device_destroy(gop_class, MKDEV(_devGOP.s32Major, _devGOP.s32Minor));
    class_destroy(gop_class);
    unregister_chrdev_region(MKDEV(_devGOP.s32Major, _devGOP.s32Minor), MDRV_GOP_DEVICE_COUNT);
    platform_driver_unregister(&Mstar_gop_driver);
}


module_init(_MDrv_GOPIO_ModuleInit);
module_exit(_MDrv_GOPIO_ModuleExit);

MODULE_AUTHOR("MSTAR");
MODULE_DESCRIPTION("GOPioctrl driver");
MODULE_LICENSE("GOP");

