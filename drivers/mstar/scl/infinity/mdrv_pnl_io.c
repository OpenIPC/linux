////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2011 MStar Semiconductor, Inc.
// All rights reserved.
//
// Unless otherwise stipulated in writing, any and all information contained
// herein regardless in any format shall remain the sole proprietary of
// MStar Semiconductor Inc. and be kept in strict confidence
// ("MStar Confidential Information") by the recipient.
// Any unauthorized act including without limitation unauthorized disclosure,
// copying, use, reproduction, sale, distribution, modification, disassembling,
// reverse engineering and compiling of the contents of MStar Confidential
// Information is unlawful and strictly prohibited. MStar hereby reserves the
// rights to any and all damages, losses, costs and expenses resulting therefrom.
//
////////////////////////////////////////////////////////////////////////////////

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
#include <asm/uaccess.h>
#include <linux/fs.h>
#include <asm/io.h>
#include <asm/string.h>

#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/poll.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/blkdev.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/wait.h>

#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include "ms_msys.h"

#include "mdrv_pnl_io_st.h"
#include "mdrv_pnl_io.h"
#include "mdrv_scl_dbg.h"
#include "mdrv_pnl.h"
#include "mdrv_verchk.h"

#define MDRV_MS_PNL_DEVICE_COUNT    1
#define MDRV_MS_PNL_NAME            "mpnl"
#define MAX_FILE_HANDLE_SUPPRT      64
#define MDRV_NAME_PNL               "mpnl"
#define MDRV_MAJOR_PNL              0xea
#define MDRV_MINOR_PNL              0x08

#define CMD_PARSING(x)  (x==IOCTL_PNL_SET_TIMING_CONFIG ?   "IOCTL_PNL_SET_TIMING_CONFIG" : \
                                                            "UNKNOWN")


int mdrv_ms_pnl_open(struct inode *inode, struct file *filp);
int mdrv_ms_pnl_release(struct inode *inode, struct file *filp);
long mdrv_ms_pnl_ioctl(struct file *filp, unsigned int u32Cmd, unsigned long u32Arg);
static int mdrv_ms_pnl_probe(struct platform_device *pdev);
static int mdrv_ms_pnl_remove(struct platform_device *pdev);
static int mdrv_ms_pnl_suspend(struct platform_device *dev, pm_message_t state);
static int mdrv_ms_pnl_resume(struct platform_device *dev);
static unsigned int mdrv_ms_pnl_poll(struct file *filp, struct poll_table_struct *wait);



//extern atomic_t SC1W_intr_count;

//-------------------------------------------------------------------------------------------------

typedef struct
{
    int s32Major;
    int s32Minor;
    int refCnt;
    struct cdev cdev;
    struct file_operations fops;
	struct device *devicenode;
}ST_DEV_PNL;

static ST_DEV_PNL _dev_ms_pnl =
{
    .s32Major = MDRV_MAJOR_PNL,
    .s32Minor = MDRV_MINOR_PNL,
    .refCnt = 0,
    .cdev =
    {
        .kobj = {.name= MDRV_NAME_PNL, },
        .owner = THIS_MODULE,
    },
    .fops =
    {
        .open = mdrv_ms_pnl_open,
        .release = mdrv_ms_pnl_release,
        .unlocked_ioctl = mdrv_ms_pnl_ioctl,
        .poll = mdrv_ms_pnl_poll,
    }
};

static struct class * m_pnl_class = NULL;
static char * pnl_classname = "m_pnl_class";


static const struct of_device_id ms_pnl_of_match_table[] =
{
    { .compatible = "mstar,pnl" },
    {}
};

static struct platform_driver st_ms_pnl_driver =
{
	.probe 		= mdrv_ms_pnl_probe,
	.remove 	= mdrv_ms_pnl_remove,
    .suspend    = mdrv_ms_pnl_suspend,
    .resume     = mdrv_ms_pnl_resume,
	.driver =
	{
		.name	= MDRV_NAME_PNL,
        .owner  = THIS_MODULE,
        .of_match_table = of_match_ptr(ms_pnl_of_match_table),
	},
};
#if (!CONFIG_OF)
static u64 ms_pnl_dma_mask = 0xffffffffUL;
static struct platform_device st_ms_pnl_device =
{
    .name = "mpnl",
    .id = 0,
    .dev =
    {
        .dma_mask = &ms_pnl_dma_mask,
        .coherent_dma_mask = 0xffffffffUL
    }
};
#endif
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
// IOCtrl Driver interface functions
//-------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
int _mdrv_ms_pnl_io_set_timing_config(struct file *filp, unsigned long arg)
{
    ST_MDRV_PNL_TIMING_CONFIG stCfg;

    if ( CHK_VERCHK_HEADER( &(((ST_IOCLT_PNL_TIMING_CONFIG __user *)arg)->VerChk_Version)) )
    {
        if( CHK_VERCHK_VERSION_LESS( &(((ST_IOCLT_PNL_TIMING_CONFIG __user *)arg)->VerChk_Version), IOCTL_PNL_VERSION) )
        {

            VERCHK_ERR("[PNL] Version(%04x) < %04x!!! \n",
                ((ST_IOCLT_PNL_TIMING_CONFIG __user *)arg)->VerChk_Version & VERCHK_VERSION_MASK,
                IOCTL_PNL_VERSION);

            return -EINVAL;
        }
        else
        {
            if( CHK_VERCHK_SIZE( &(((ST_IOCLT_PNL_TIMING_CONFIG __user *)arg)->VerChk_Size), sizeof(ST_IOCLT_PNL_TIMING_CONFIG)) == 0 )
            {
                VERCHK_ERR("[PNL] Size(%04x) != %04x!!! \n",
                    sizeof(ST_IOCLT_PNL_TIMING_CONFIG),
                    (((ST_IOCLT_PNL_TIMING_CONFIG __user *)arg)->VerChk_Size));

                return -EINVAL;
            }
            else
            {
                if(copy_from_user(&stCfg, &(((ST_IOCLT_PNL_TIMING_CONFIG __user *)arg)->u16Vsync_St), sizeof(ST_MDRV_PNL_TIMING_CONFIG)))
                {
                    return -EFAULT;
                }
            }
        }
    }
    else
    {
        VERCHK_ERR("[PNL] No Header !!! \n");

        if(copy_from_user(&stCfg, (ST_MDRV_PNL_TIMING_CONFIG __user *)arg, sizeof(ST_MDRV_PNL_TIMING_CONFIG)))
        {
            return -EFAULT;
        }
    }
    SCL_DBG(SCL_DBG_LV_IOCTL()&EN_DBGMG_IOCTLEVEL_ELSE, "[PNL] Size(%04d) \n",(((ST_IOCLT_PNL_TIMING_CONFIG __user *)arg)->VerChk_Size) );


    if(!MDrv_PNL_Set_Timing_Config(&stCfg))
    {
        return -EFAULT;
    }

    return 0;
}

int _mdrv_ms_pnl_io_get_version(struct file *filp, unsigned long arg)
{
    int ret = 0;

    if (CHK_VERCHK_HEADER( &(((ST_IOCTL_PNL_VERSION_CONFIG __user *)arg)->VerChk_Version)) )
    {
        if( CHK_VERCHK_VERSION_LESS( &(((ST_IOCTL_PNL_VERSION_CONFIG __user *)arg)->VerChk_Version), IOCTL_PNL_VERSION) )
        {

            VERCHK_ERR("[PNL] Version(%04x) < %04x!!! \n",
                ((ST_IOCTL_PNL_VERSION_CONFIG __user *)arg)->VerChk_Version & VERCHK_VERSION_MASK,
                IOCTL_PNL_VERSION);

            ret = -EINVAL;
        }
        else
        {
            if( CHK_VERCHK_SIZE( &(((ST_IOCTL_PNL_VERSION_CONFIG __user *)arg)->VerChk_Size), sizeof(ST_IOCTL_PNL_VERSION_CONFIG)) == 0 )
            {
                VERCHK_ERR("[PNL] Size(%04x) != %04x!!! \n",
                    sizeof(ST_IOCTL_PNL_VERSION_CONFIG),
                    (((ST_IOCTL_PNL_VERSION_CONFIG __user *)arg)->VerChk_Size));

                ret = -EINVAL;
            }
            else
            {
                ST_IOCTL_PNL_VERSION_CONFIG stCfg;

                stCfg = FILL_VERCHK_TYPE(stCfg, stCfg.VerChk_Version, stCfg.VerChk_Size, IOCTL_PNL_VERSION);
                SCL_DBG(SCL_DBG_LV_IOCTL()&EN_DBGMG_IOCTLEVEL_ELSE, "[PNL] Size(%04d) \n",(((ST_IOCTL_PNL_VERSION_CONFIG __user *)arg)->VerChk_Size) );
                stCfg.u32Version = IOCTL_PNL_VERSION;

                if(copy_to_user((ST_IOCTL_PNL_VERSION_CONFIG __user *)arg, &stCfg, sizeof(ST_IOCTL_PNL_VERSION_CONFIG)))
                {
                    ret = -EFAULT;
                }
                else
                {
                    ret = 0;
                }
            }
        }
    }
    else
    {
        VERCHK_ERR("[PNL] No Header !!! \n");
        ret = -EINVAL;
    }

    return ret;
}


//==============================================================================
long mdrv_ms_pnl_ioctl(struct file *filp, unsigned int u32Cmd, unsigned long u32Arg)
{
    int err = 0;
    int retval = 0;

    if(_dev_ms_pnl.refCnt <= 0)
    {
        SCL_ERR( "[PNL] PNLIO_IOCTL refCnt =%d!!! \n", _dev_ms_pnl.refCnt);
        return -EFAULT;
    }
    /* check u32Cmd valid */
    if(IOCTL_PNL_MAGIC == _IOC_TYPE(u32Cmd))
    {
        if(_IOC_NR(u32Cmd) >= IOCTL_PNL_MAX_NR)
        {
            SCL_ERR( "[PNL] IOCtl NR Error!!! (Cmd=%x)\n",u32Cmd);
            return -ENOTTY;
        }
    }
    else
    {
        SCL_ERR( "[PNL] IOCtl MAGIC Error!!! (Cmd=%x)\n",u32Cmd);
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

    SCL_DBG(SCL_DBG_LV_IOCTL()&EN_DBGMG_IOCTLEVEL_ELSE, "[PNL] IOCTL_NUM:: == %s ==  \n", (CMD_PARSING(u32Cmd)));

    switch(u32Cmd)
    {
    case IOCTL_PNL_SET_TIMING_CONFIG:
        retval = _mdrv_ms_pnl_io_set_timing_config(filp, u32Arg);
        break;

    case IOCTL_PNL_GET_VERSION_CONFIG:
        retval = _mdrv_ms_pnl_io_get_version(filp, u32Arg);
        break;

    default:  /* redundant, as cmd was checked against MAXNR */
        SCL_ERR( "[PNL] ERROR IOCtl number %x\n ",u32Cmd);
        retval = -ENOTTY;
        break;
    }

    return retval;
}


static unsigned int mdrv_ms_pnl_poll(struct file *filp, struct poll_table_struct *wait)
{

    return 0;
}
#if CONFIG_OF
static int mdrv_ms_pnl_probe(struct platform_device *pdev)
{
    ST_MDRV_PNL_INIT_CONFIG stPnlInitCfg;
    int s32Ret;
    dev_t  dev;
    SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[PNL] %s\n",__FUNCTION__);
    if(_dev_ms_pnl.s32Major)
    {
        dev = MKDEV(_dev_ms_pnl.s32Major, _dev_ms_pnl.s32Minor);
        s32Ret = register_chrdev_region(dev, MDRV_MS_PNL_DEVICE_COUNT, MDRV_MS_PNL_NAME);
    }
    else
    {
        s32Ret = alloc_chrdev_region(&dev, _dev_ms_pnl.s32Minor, MDRV_MS_PNL_DEVICE_COUNT, MDRV_MS_PNL_NAME);
        _dev_ms_pnl.s32Major = MAJOR(dev);
    }

    if (0 > s32Ret)
    {
        SCL_ERR( "[PNL] Unable to get major %d\n", _dev_ms_pnl.s32Major);
        return s32Ret;
    }

    cdev_init(&_dev_ms_pnl.cdev, &_dev_ms_pnl.fops);
    if (0 != (s32Ret= cdev_add(&_dev_ms_pnl.cdev, dev, MDRV_MS_PNL_DEVICE_COUNT)))
    {
        SCL_ERR( "[PNL] Unable add a character device\n");
        unregister_chrdev_region(dev, MDRV_MS_PNL_DEVICE_COUNT);
        return s32Ret;
    }

    m_pnl_class = msys_get_sysfs_class();
    if(!m_pnl_class)
    {
        m_pnl_class = class_create(THIS_MODULE, pnl_classname);
    }
    if(IS_ERR(m_pnl_class))
    {
        printk(KERN_WARNING"Failed at class_create().Please exec [mknod] before operate the device/n");
    }
    else
    {
        _dev_ms_pnl.devicenode =device_create(m_pnl_class, NULL, dev,NULL, "mpnl");
    }
    stPnlInitCfg.u32RiuBase = 0x1F000000;
    if(MDrv_PNL_Init(&stPnlInitCfg) == 0)
    {
        return -EFAULT;
    }
    gbProbeAlready |= EN_DBG_PNL_CONFIG;
    return 0;
}

static int mdrv_ms_pnl_remove(struct platform_device *pdev)
{
    SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[PNL] %s\n",__FUNCTION__);
    cdev_del(&_dev_ms_pnl.cdev);
    device_destroy(m_pnl_class, MKDEV(_dev_ms_pnl.s32Major, _dev_ms_pnl.s32Minor));
    class_destroy(m_pnl_class);
    unregister_chrdev_region(MKDEV(_dev_ms_pnl.s32Major, _dev_ms_pnl.s32Minor), MDRV_MS_PNL_DEVICE_COUNT);
    return 0;
}
#else
static int mdrv_ms_pnl_probe(struct platform_device *pdev)
{
    ST_MDRV_PNL_INIT_CONFIG stPnlInitCfg;

    SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[PNL] %s\n",__FUNCTION__);

    stPnlInitCfg.u32RiuBase = 0x1F000000;
    if(MDrv_PNL_Init(&stPnlInitCfg) == 0)
    {
        return -EFAULT;
    }

    return 0;
}

static int mdrv_ms_pnl_remove(struct platform_device *pdev)
{
    SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[PNL] %s\n",__FUNCTION__);

    return 0;
}
#endif

static int mdrv_ms_pnl_suspend(struct platform_device *dev, pm_message_t state)
{
    SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[PNL] %s\n",__FUNCTION__);
    return 0;
}


static int mdrv_ms_pnl_resume(struct platform_device *dev)
{
    SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[PNL] %s\n",__FUNCTION__);
    MDrv_PNL_Resume();
    return 0;
}


int mdrv_ms_pnl_open(struct inode *inode, struct file *filp)
{
    SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[PNL] %s\n",__FUNCTION__);

    SCL_ASSERT(_dev_ms_pnl.refCnt>=0);
    _dev_ms_pnl.refCnt++;



    return 0;
}


int mdrv_ms_pnl_release(struct inode *inode, struct file *filp)
{

    SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[PNL] %s\n",__FUNCTION__);
    _dev_ms_pnl.refCnt--;
    SCL_ASSERT(_dev_ms_pnl.refCnt>=0);
    if(_dev_ms_pnl.refCnt==0)
    {
        MDrv_PNL_Release();
    }
    //free_irq(INT_IRQ_PNLW, MDrv_PNLW_isr);

    return 0;
}


//-------------------------------------------------------------------------------------------------
// Module functions
//-------------------------------------------------------------------------------------------------
#if CONFIG_OF
int _mdrv_ms_pnl_init(void)
{
    int ret = 0;
    SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[PNL] %s:%d\n",__FUNCTION__,__LINE__);
    ret = platform_driver_register(&st_ms_pnl_driver);
    if (!ret)
    {
        SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[PNL] platform_driver_register success\n");
        if(gbProbeAlready&EN_DBG_PNL_CONFIG)
        {
            SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[PNL] Probe success\n");
        }
        else
        {
            SCL_ERR( "[PNL] Probe Fail\n");
        }
        if(gbProbeAlready==EN_DBG_SCL_CONFIG)
        {
            SCL_ERR( "[SCL] SCL init success\n");
        }
    }
    else
    {
        SCL_ERR( "[PNL] platform_driver_register failed\n");
        platform_driver_unregister(&st_ms_pnl_driver);
    }

    return ret;
}
void _mdrv_ms_pnl_exit(void)
{
    /*de-initial the who GFLIPDriver */
    SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[PNL] %s\n",__FUNCTION__);
    platform_driver_unregister(&st_ms_pnl_driver);
}
#else
int _mdrv_ms_pnl_init(void)
{
    int ret = 0;
    int s32Ret;
    dev_t  dev;

    SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[PNL] %s\n",__FUNCTION__);

    if(_dev_ms_pnl.s32Major)
    {
        dev = MKDEV(_dev_ms_pnl.s32Major, _dev_ms_pnl.s32Minor);
        s32Ret = register_chrdev_region(dev, MDRV_MS_PNL_DEVICE_COUNT, MDRV_MS_PNL_NAME);
    }
    else
    {
        s32Ret = alloc_chrdev_region(&dev, _dev_ms_pnl.s32Minor, MDRV_MS_PNL_DEVICE_COUNT, MDRV_MS_PNL_NAME);
        _dev_ms_pnl.s32Major = MAJOR(dev);
    }

    if (0 > s32Ret)
    {
        SCL_ERR( "[PNL] Unable to get major %d\n", _dev_ms_pnl.s32Major);
        return s32Ret;
    }

    cdev_init(&_dev_ms_pnl.cdev, &_dev_ms_pnl.fops);
    if (0 != (s32Ret= cdev_add(&_dev_ms_pnl.cdev, dev, MDRV_MS_PNL_DEVICE_COUNT)))
    {
        SCL_ERR( "[PNL] Unable add a character device\n");
        unregister_chrdev_region(dev, MDRV_MS_PNL_DEVICE_COUNT);
        return s32Ret;
    }

    m_pnl_class = class_create(THIS_MODULE, pnl_classname);
    if(IS_ERR(m_pnl_class))
    {
        printk(KERN_WARNING"Failed at class_create().Please exec [mknod] before operate the device/n");
    }
    else
    {
        device_create(m_pnl_class, NULL, dev,NULL, "mpnl");
    }

    /* initial the whole PNL Driver */
    ret = platform_driver_register(&st_ms_pnl_driver);

    if (!ret)
    {
        ret = platform_device_register(&st_ms_pnl_device);
        if (ret)    /* if register device fail, then unregister the driver.*/
        {
            platform_driver_unregister(&st_ms_pnl_driver);
            SCL_ERR( "[PNL] platform_driver_register failed\n");

        }
        else
        {
            SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[PNL] platform_driver_register success\n");
        }
    }


    return ret;
}


void _mdrv_ms_pnl_exit(void)
{
    /*de-initial the who GFLIPDriver */
    SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[PNL] %s\n",__FUNCTION__);

    cdev_del(&_dev_ms_pnl.cdev);
    device_destroy(m_pnl_class, MKDEV(_dev_ms_pnl.s32Major, _dev_ms_pnl.s32Minor));
    class_destroy(m_pnl_class);
    unregister_chrdev_region(MKDEV(_dev_ms_pnl.s32Major, _dev_ms_pnl.s32Minor), MDRV_MS_PNL_DEVICE_COUNT);
    platform_driver_unregister(&st_ms_pnl_driver);
}
#endif

module_init(_mdrv_ms_pnl_init);
module_exit(_mdrv_ms_pnl_exit);

MODULE_AUTHOR("MSTAR");
MODULE_DESCRIPTION("ms pnl ioctrl driver");
MODULE_LICENSE("GPL");
