////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2017 MStar Semiconductor, Inc.
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

#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/spinlock.h>

#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/poll.h>

#include <linux/clk.h>
#include <linux/clk-provider.h>

#include "ms_msys.h"

#include "hal_jpe_ios.h"
#include "hal_jpe_ops.h"
#include "_drv_jpe_dev.h"
#include "_drv_jpe_ctx.h"
#include "drv_jpe_module.h"

#define MDRV_JPE_DEVICE_COUNT   1               // How many device will be installed
#define MDRV_JPE_NAME           "mstar_jpe"
#define MDRV_JPE_MINOR          0
#define MDRV_JPE_CLASS_NAME     "mstar_jpe_class"

#define JPE_CLOCK_ON    1
#define JPE_CLOCK_OFF   0

static JpeDev_t* _gpDev = NULL;

//=============================================================================
// Description:
//     ISR handler
// Author:
//      Albert.Liao.
// Input:
//   irq:      IRQ
//   dev_data: Device data which is assigned from request_irq()
// Output:
//   Always IRQ_HANDLED to stop parsing IS
//=============================================================================
irqreturn_t  _JpeIsr(int irq, void* pDevData)
{
    JpeDev_t* pDev = (JpeDev_t*)pDevData;

    if(pDev)
    {
        JpeDevIsrFnx(pDev);
    }
    else
    {
        JPE_MSG(JPE_MSG_ERR, "pDev==NULL, JPE device is removed\n");
    }

    return IRQ_HANDLED;
}


//=============================================================================
// _DevPowserOn
//     Turn on/off clock
//
// Parameters:
//     pdev: platform device
//     bOn:  use 1 to turn on clock, and use 0 to turn off clock
//
// Return:
//     standard return value
//=============================================================================
static JPE_IOC_RET_STATUS_e _DevPowserOn(JpeDev_t* pDev, int bOn)
{
    int i = 0;
    int nRet;
    struct clk* pClock;

    if(!pDev)
        return JPE_IOC_RET_FAIL;

    if(JPE_CLOCK_ON == bOn)
    {
        while (i < JPE_CLOCKS_NR && (pClock = pDev->pClock[i++]))
            clk_prepare_enable(pClock);
        pClock = pDev->pClock[0];
        nRet = clk_set_rate(pClock, pDev->nClockRate);
        if(nRet)
        {
            JPE_MSG(JPE_MSG_ERR, "clk_set_rate() Fail, ret=%d\n", nRet);
            return JPE_IOC_RET_FAIL;
        }
        nRet = clk_prepare_enable(pClock);
        if(nRet)
        {
            JPE_MSG(JPE_MSG_ERR, "clk_prepare_enable() Fail, ret=%d\n", nRet);
            return JPE_IOC_RET_FAIL;
        }
        pDev->i_users++;
    }
    else
    {
        pClock = pDev->pClock[0];
        clk_set_parent(pClock, clk_get_parent_by_index(pClock, 0));
        while (i < JPE_CLOCKS_NR && (pClock = pDev->pClock[i++]))
        clk_disable_unprepare(pClock);
        pDev->i_users--;
    }

    return JPE_IOC_RET_SUCCESS;
}

//=============================================================================
// Description:
//     File open handler
// Author:
//      Albert.Liao.
// Input:
//      pDev: JPEG hardware device handle
//      pCtx: JPEG user handle
// Output:
//      0: success
//
//=============================================================================
int JpeOpen(struct inode *inode, struct file *filp)
{
    JpeDev_t* pDev = NULL;
    JpeCtx_t* pCtx = NULL;
    int nRet, nStatus = 0;

    pDev = container_of(inode->i_cdev, JpeDev_t, tCharDev);
    if(!pDev)
    {
        JPE_MSG(JPE_MSG_ERR, "pDev==NULL\n");
        goto _OpenErr;
    }

    pCtx = JpeCtxAcquire(pDev);
    if(pCtx == NULL)
    {
        JPE_MSG(JPE_MSG_ERR, "JpeCtxAcquire Fail\n");
        nStatus = -ENODEV;
	return nStatus;
        //goto _OpenErr;
    }

    // After success register, pDev->user[0].pCtx = pCtx
    nRet = JpeDevRegister(pDev, pCtx);
    if(0 > nRet)
    {
        JPE_MSG(JPE_MSG_ERR, "JpeDevRegister Fail, ret=%d\n", nRet);
        nStatus = -EUSERS;
        goto _OpenErr;
    }

    pDev->nRefCount++;
    filp->private_data = pCtx;

    nRet = _DevPowserOn(pDev, JPE_CLOCK_ON);
    if(nRet)
    {
        JPE_MSG(JPE_MSG_ERR, "clk_set_rate() Fail, ret=%d\n", nRet);
        nStatus = -ENODEV;
        goto _OpenErr;
    }

    return JPE_IOC_RET_SUCCESS;

_OpenErr:
    JPE_MSG(JPE_MSG_ERR, "JpeOpen Fail \n");
    pCtx->release(pCtx);

    return nStatus;
}


//=============================================================================
// Description:
//     File close handler
// Author:
//      Albert.Liao.
// Input:
//      pDev: JPEG hardware device handle
//      pCtx: JPEG user handle
// Output:
//      JPE_IOC_RET_SUCCESS: Success
//      JPE_IOC_RET_FAIL: Failure
//=============================================================================
int JpeRelease(struct inode *inode, struct file *filp)
{
    JpeDev_t* pDev = NULL;
    JpeCtx_t* pCtx = NULL;
    JPE_IOC_RET_STATUS_e eStatus;
    int nRet;

    pCtx = filp->private_data;
    if (NULL == pCtx)
    {
        return -ENOMEDIUM;
    }

    pDev = pCtx->p_device;

    nRet = _DevPowserOn(pDev, JPE_CLOCK_OFF);
    if(nRet)
    {
        JPE_MSG(JPE_MSG_ERR, "clk_set_rate() Fail, ret=%d\n", nRet);
        return -ENODEV;
    }

    eStatus = JpeDevUnregister(pDev, pCtx);
    if(JPE_IOC_RET_SUCCESS != eStatus)
    {
        JPE_MSG(JPE_MSG_ERR, "JpeDevUnregister Fail ret=%d\n", nRet);
        return -ENOMEDIUM;
    }

    if(pCtx->release)
    {
        pCtx->release(pCtx);
    }

    return JPE_IOC_RET_SUCCESS;
}


//=============================================================================
// Description:
//     IOCTL handler entry for file operator
// Author:
//      Albert.Liao.
// Input:
//      filp: pointer of file structure
//      cmd:  command
//      arg:  argument from user-space
// Output:
//      JPE_IOC_RET_SUCCESS: Success
//      JPE_IOC_RET_FAIL: Failure
//=============================================================================
long JpeIoctl(struct file *pFilp, unsigned int nCmd, unsigned long nArg)
{
    JpeCtx_t* pCtx = (JpeCtx_t*)pFilp->private_data;
    JPE_IOC_RET_STATUS_e eStatus = JPE_IOC_RET_SUCCESS;
#if 0
    switch(nCmd)
    {
        case JPE_IOC_INIT:
            JPE_MSG(JPE_MSG_ERR, "JpeIoctl command = JPE_IOC_INIT\n");
            break;
        case JPE_IOC_ENCODE_FRAME:
            JPE_MSG(JPE_MSG_ERR, "JpeIoctl command = JPE_IOC_ENCODE_FRAME\n");
            break;
        case JPE_IOC_GETBITS:
            JPE_MSG(JPE_MSG_ERR, "JpeIoctl command = JPE_IOC_GETBITS\n");
            break;
        case JPE_IOC_GET_CAPS:
            JPE_MSG(JPE_MSG_ERR, "JpeIoctl command = JPE_IOC_GET_CAPS\n");
            break;
        case JPE_IOC_SET_OUTBUF:
            JPE_MSG(JPE_MSG_ERR, "JpeIoctl command = JPE_IOC_SET_OUTBUF\n");
            break;
    }
#endif

    switch(nCmd)
    {
        case JPE_IOC_INIT:
            //_DumpConfig((JpeCfg_t *)nArg);
            eStatus = JpeCtxActions(pCtx, JPE_IOC_INIT, (void *)nArg);
            break;

        case JPE_IOC_SET_CLOCKRATE:
        {
            int nRet = 0, nClkSelect = (int)(nArg);
            JpeDev_t* pDev = pCtx->p_device;
            struct clk* pClock;

            // 0: 288MHz  1: 216MHz  2: 54MHz  3: 27MHz
            if((nClkSelect>=0) && (nClkSelect<4))
            {
                pClock = pDev->pClock[0];
                if(pClock)
                {
                    pDev->nClockRate = clk_get_rate(clk_get_parent_by_index(pClock, nClkSelect));
                    nRet = clk_set_rate(pClock, pDev->nClockRate);
                    if(nRet)
                    {
                        eStatus = JPE_IOC_RET_FAIL;
                    }
                }
            }
            break;
        }
        case JPE_IOC_ENCODE_FRAME:
            eStatus = JpeCtxActions(pCtx, JPE_IOC_ENCODE_FRAME, (void *)nArg);
            break;

        case JPE_IOC_SET_OUTBUF:
        {
            JpeBufInfo_t        *pOutBuf = NULL;
            pOutBuf = (JpeBufInfo_t *)nArg;
            //JPE_MSG(JPE_MSG_ERR, "pOutBuf nAddr=0x%x, size=%d\n",  (int)pOutBuf->nAddr, (int)pOutBuf->nSize);
            eStatus = JpeCtxActions(pCtx, JPE_IOC_SET_OUTBUF, (void *)nArg);
            break;
        }

        case JPE_IOC_GETBITS:
            eStatus = JpeCtxActions(pCtx, JPE_IOC_GETBITS, (void *)nArg);
            //_DumpBitInfo((JpeBitstreamInfo_t *)nArg);
            break;

        case JPE_IOC_GET_CAPS:
            eStatus = JpeCtxActions(pCtx, JPE_IOC_GET_CAPS, (void *)nArg);
            break;

        default:
            eStatus = JPE_IOC_RET_FAIL;
            break;
    }

    // For linux ioctl usage,
    // If error happens, return value is always -1, and the actual error code is stored in errno
    if(JPE_IOC_RET_SUCCESS != eStatus)
        eStatus = -eStatus;

    return eStatus;
}


//-------------------------------------------------------------------------------------------------
// Platform functions
//-------------------------------------------------------------------------------------------------

// Use a struct to gather all global variable
static struct
{
    int major;              // cdev major number
    int minor_start;         // beginning of cdev minor number
    int reg_count;          // registered count
    struct class *class;    // class pointer
} g_jpe_drv = {0, 0, 0, NULL};

static const struct file_operations jpe_fops =
{
    .owner          = THIS_MODULE,
    .open           = JpeOpen,
    .release        = JpeRelease,
    .unlocked_ioctl = JpeIoctl,
};

//static void _JpeDevRelease(struct device* dev) {}

//=============================================================================
// Description:
//     Platform device probe handler
// Author:
//      Albert.Liao.
// Input:
//      pDev: JPEG hardware device handle
// Output:
//      JpeDev_t: JPEG hardware device handle
//=============================================================================
//JpeDev_t* JpeProbe(JpeDev_t* pDev)
static int JpeProbe(struct platform_device *pPlatformDev)
{
    int i;
    JpeDev_t* pDev = NULL;
    JpeIosCB_t* mios = NULL;
    JpeRegIndex_t mregs;

    dev_t dev;
    struct resource *pResource;
    struct clk* pClock;

    int nErr = 0;
    JPE_IOC_RET_STATUS_e eStatus;

    if(_gpDev)
        return 0;

    /* Allocate cdev id */
    nErr = alloc_chrdev_region(&dev, MDRV_JPE_MINOR, MDRV_JPE_DEVICE_COUNT, MDRV_JPE_NAME);
    if(nErr)
    {
        JPE_MSG(JPE_MSG_ERR, "Unable allocate cdev id\n");
        return nErr;
    }

    g_jpe_drv.major = MAJOR(dev);
    g_jpe_drv.minor_start = MINOR(dev);
    g_jpe_drv.reg_count = 0;

    /* Register device class */
    g_jpe_drv.class = (struct class *)msys_get_sysfs_class();
    if(IS_ERR(g_jpe_drv.class))
    {
        JPE_MSG(JPE_MSG_ERR, "Failed at class_create().Please exec [mknod] before operate the device\n");
        nErr = PTR_ERR(g_jpe_drv.class);
        return nErr;
    }

    do
    {
        pDev = CamOsMemCalloc(1, sizeof(JpeDev_t));
        if(pDev == NULL)
        {
            JPE_MSG(JPE_MSG_ERR, "Create Mdev Fail \n");
            goto _ProbeErr;
        }

        pDev->pPlatformDev = pPlatformDev;

        CamOsMutexInit(&pDev->m_mutex);
        CamOsTsemInit(&pDev->tGetBitsSem, 1);
        CamOsTsemInit(&pDev->m_wqh, JPE_DEV_STATE_IDLE);

        pDev->p_asicip = JpeIosAcquire("jpe");
        mios = pDev->p_asicip;
        if(mios == NULL)
        {
            JPE_MSG(JPE_MSG_ERR, "Create Mios Fail \n");
            goto _ProbeErr;
        }

        /* memory-mapped IO registers */
        pResource = platform_get_resource(pPlatformDev, IORESOURCE_MEM, 0);
        if(!pResource)
        {
            goto _ProbeErr;
        }

        mregs.i_id = 0;
        mregs.base = (void*)IO_ADDRESS(pResource->start);
        mregs.size = (int)(pResource->end - pResource->start);

        eStatus = mios->setBank(mios, &mregs);
        if(JPE_IOC_RET_SUCCESS != eStatus)
        {
            JPE_MSG(JPE_MSG_ERR, "> set_bank Fail \n");
            goto _ProbeErr;
        }

        /* IRQ registration */
        pDev->irq = irq_of_parse_and_map(pPlatformDev->dev.of_node, 0);
        if(pDev->irq == 0)
        {
            JPE_MSG(JPE_MSG_ERR, "Cannot find IRQ\n");
            goto _ProbeErr;
        }

        pClock = clk_get(&pPlatformDev->dev, "CKG_jpe");
        if (IS_ERR(pClock))
        {
            JPE_MSG(JPE_MSG_ERR, "clk_get failed\n");
            goto _ProbeErr;
        }
        pDev->pClock[0] = pClock;
        pDev->nClockIdx = 0; // 0: 288MHz  1: 216MHz  2: 54MHz  3: 27MHz
        pDev->nClockRate = clk_get_rate(clk_get_parent_by_index(pClock, pDev->nClockIdx));
        JPE_MSG(JPE_MSG_ERR, "set base=0x%x irq=%d, nClockRate=%d\n", (u32)mregs.base, pDev->irq, (u32)pDev->nClockRate);
        for (i = 1; i < JPE_CLOCKS_NR; i++)
        {
            pClock = of_clk_get(pPlatformDev->dev.of_node, i);
            if (IS_ERR(pClock))
                break;
            pDev->pClock[i] = pClock;
        }

        /* Add cdev */
        cdev_init(&pDev->tCharDev, &jpe_fops);
        pDev->tCharDev.owner = THIS_MODULE;
        nErr = cdev_add(&pDev->tCharDev, MKDEV(g_jpe_drv.major, g_jpe_drv.minor_start + g_jpe_drv.reg_count), 1);
        if(nErr)
        {
            JPE_MSG(JPE_MSG_ERR, "Unable add a character device\n");
            goto _ProbeErr;
        }

        /* Create a instance in class */
        device_create(g_jpe_drv.class,
                      NULL,
                      MKDEV(g_jpe_drv.major, g_jpe_drv.minor_start + g_jpe_drv.reg_count),
                      pDev,
                      MDRV_JPE_NAME"%d", g_jpe_drv.minor_start + g_jpe_drv.reg_count);

        /* Increase registered count */
        g_jpe_drv.reg_count++;

        // Register a ISR
        //nRet = request_irq(pDev->irq, _JpeIsr, IRQF_SHARED, "jpe interrupt", pDev);
        nErr = request_irq(pDev->irq, _JpeIsr, IRQ_TYPE_LEVEL_HIGH, "jpe interrupt", pDev);
        if (nErr)
        {
            JPE_MSG(JPE_MSG_ERR, "request_irq() failed (%d)\n", nErr);
            goto _ProbeErr;
        }

        dev_set_drvdata(&pPlatformDev->dev, pDev);

        _gpDev = pDev;
    }
    while(0);

    return nErr;

//TODO: fix me
_ProbeErr:
    if(mios != NULL)
        CamOsMemRelease(mios);
    if(pDev != NULL)
        CamOsMemRelease(pDev);
    return -ENODEV;
}


//=============================================================================
// Description:
//     File close handler
// Author:
//      Albert.Liao.
// Input:
//      pDev: JPEG hardware device handle
// Output:
//      JPE_IOC_RET_SUCCESS: Success
//      JPE_IOC_RET_FAIL: Failure
//=============================================================================
static int JpeRemove(struct platform_device *pPlatformDev)
{
    JpeDev_t* pDev = NULL;
    JpeIosCB_t* pIos = NULL;

    pDev = dev_get_drvdata(&pPlatformDev->dev);

    if(pDev)
    {
        CamOsMutexDestroy(&pDev->m_mutex);
        CamOsTsemDeinit(&pDev->m_wqh);
        CamOsTsemDeinit(&pDev->tGetBitsSem);
        pIos = pDev->p_asicip;
    }
    else
    {
        JPE_MSG(JPE_MSG_ERR, "pDev==NULL, %s fail\n", __func__);
        return JPE_IOC_RET_FAIL;
    }

    if(pIos)
    {
        // TODO: Should I do irq mask???
        //pIos->irqMask(pIos, 0xFF);
        pIos->release(pIos);
    }

    free_irq(pDev->irq, pDev);

    // TODO: check g_jpe_drv.reg_count before destory device
    device_destroy(g_jpe_drv.class, pDev->tCharDev.dev);
    cdev_del(&pDev->tCharDev);

    CamOsMemRelease(pDev);

    unregister_chrdev_region(MKDEV(g_jpe_drv.major, g_jpe_drv.minor_start), MDRV_JPE_DEVICE_COUNT);

    _gpDev = NULL;
    return JPE_IOC_RET_SUCCESS;

}


//=============================================================================
// JpeSuspend
//     Platform device suspend handler, but nothing to do here
//
// Parameters:
//     pPlatformDev: platform device
//     state: don't use
//
// Return:
//     standard return value
//=============================================================================
static int JpeSuspend(struct platform_device *pPlatformDev, pm_message_t state)
{
    int nRet;
    JpeDev_t* pDev = NULL;

    pDev = dev_get_drvdata(&pPlatformDev->dev);

    CamOsTsemDown(&pDev->tGetBitsSem);
    if(pDev->i_users > 0)
    {
        nRet = _DevPowserOn(pDev, JPE_CLOCK_OFF);
        if(nRet)
        {
            JPE_MSG(JPE_MSG_ERR, "Power can not turn off, ret=%d\n", nRet);
            return -ENODEV;
        }
    }

    return 0;
}


//=============================================================================
// JpeResume
//     Platform device resume handler, but nothing to do here
//
// Parameters:
//     pPlatformDev: platform device
//
// Return:
//     standard return value
//=============================================================================
static int JpeResume(struct platform_device *pPlatformDev)
{
    int nRet;
    JpeDev_t* pDev = NULL;

    pDev = dev_get_drvdata(&pPlatformDev->dev);

    if(pDev->i_users > 0)
    {
        nRet = _DevPowserOn(pDev, JPE_CLOCK_ON);
        if(nRet)
        {
            JPE_MSG(JPE_MSG_ERR, "Power can not turn on, ret=%d\n", nRet);
            CamOsTsemUp(&pDev->tGetBitsSem);
            return -ENODEV;
        }
    }
    CamOsTsemUp(&pDev->tGetBitsSem);

    return 0;
}

//-------------------------------------------------------------------------------------------------
// Data structure for device driver
//-------------------------------------------------------------------------------------------------
static const struct of_device_id mdrv_jpeg_match[] =
{
    {
            .compatible = "mstar,cedric-jpe",
            /*.data = NULL,*/
    },
    {
            .compatible = "mstar,infinity-jpe",
            /*.data = NULL,*/
    },
    {},
};


static struct platform_driver mdrv_jpe_driver =
{
    .probe      = JpeProbe,
    .remove     = JpeRemove,
    .suspend    = JpeSuspend,
    .resume     = JpeResume,

    .driver =
    {
        .of_match_table = of_match_ptr(mdrv_jpeg_match),
        .name   = "mstar_jpe",
        .owner  = THIS_MODULE,
    }
};


/*******************************************************************************************************************
 * _mdrv_jpe_module_init
 *   module init function
 *
 * Parameters:
 *   N/A
 *
 * Return:
 *   standard return value
 */
int mdrv_jpe_module_init(void)
{
    JPE_MSG(JPE_MSG_DEBUG, "Module Init\n");
    /* Register platform driver */
    return platform_driver_register(&mdrv_jpe_driver);
}



/*******************************************************************************************************************
 * mdrv_jpe_module_exit
 *   module exit function
 *
 * Parameters:
 *   N/A
 *
 * Return:
 *   standard return value
 */
void mdrv_jpe_module_exit(void)
{
    /* de-initial the who GFLIPDriver */
    JPE_MSG(JPE_MSG_ERR, "Module Exit\n");
    platform_driver_unregister(&mdrv_jpe_driver);
}

module_init(mdrv_jpe_module_init);
module_exit(mdrv_jpe_module_exit);

MODULE_AUTHOR("MSTAR");
MODULE_DESCRIPTION("JPE ioctrl driver");
MODULE_LICENSE("GPL");