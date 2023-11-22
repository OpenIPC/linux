/*
 * clk_ut_module.c- Sigmastar
 *
 * Copyright (c) [2019~2020] SigmaStar Technology.
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License version 2 for more details.
 *
 */

#include <linux/pfn.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/vmalloc.h> /* seems do not need this */
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/sched.h>

#include <linux/module.h>
#include <linux/kernel.h>
#include <asm/uaccess.h>
#include <linux/fs.h>
#include <asm/io.h>
#include <asm/string.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/poll.h>
#include <linux/irqreturn.h>
#include <linux/interrupt.h>
#include <linux/of_address.h>

//#include "irqs.h"
#include "ms_platform.h"
#include "ms_msys.h"
#include "cam_sysfs.h"
#include "cam_os_wrapper.h"
#include "drv_camclk_Api.h"
#include "drv_camclk_DataType.h"
#include "drv_camclk.h"
#include "camclk_dbg.h"
//-------------------------------------------------------------------------------------------------
// Define & Macro
//-------------------------------------------------------------------------------------------------

#define DRV_CAMCLKUT_DEVICE_COUNT 1
#define DRV_CAMCLKUT_DEVICE_NAME  "camclkut"
#define DRV_CAMCLKUT_DEVICE_MAJOR 0x8a
#define DRV_CAMCLKUT_DEVICE_MINOR 0x0
#define DRV_CAMCLKUT_DEVICE_NODE  "camdriver,camclkut"
//-------------------------------------------------------------------------------------------------
// Prototype
//-------------------------------------------------------------------------------------------------

static int DrvCamClkUtModuleProbe(struct platform_device *pdev);
static int DrvCamClkUtModuleRemove(struct platform_device *pdev);
u32        DrvCamclkUtShow(char *buf);
void       DrvCamclkUtCmdParser(const char *buf, u32 n);
//-------------------------------------------------------------------------------------------------
// Structure
//-------------------------------------------------------------------------------------------------

typedef struct
{
    int                    s32Major;
    int                    s32Minor;
    int                    refCnt;
    int                    binit;
    struct cdev            cdev;
    struct file_operations fops;
    struct device *        devicenode;
} DrvCamClkUtModuleDevice_t;

//-------------------------------------------------------------------------------------------------
// Variable
//-------------------------------------------------------------------------------------------------

static DrvCamClkUtModuleDevice_t m_stCamClkUtDevice = {
    .s32Major   = DRV_CAMCLKUT_DEVICE_MAJOR,
    .s32Minor   = DRV_CAMCLKUT_DEVICE_MINOR,
    .refCnt     = 0,
    .binit      = 0,
    .devicenode = NULL,
    .cdev =
        {
            .kobj =
                {
                    .name = DRV_CAMCLKUT_DEVICE_NAME,
                },
            .owner = THIS_MODULE,
        },
};

static struct class *m_pstCamClkUtClass = NULL;

static const struct of_device_id m_stCamClkUtMatchTable[] = {{.compatible = DRV_CAMCLKUT_DEVICE_NODE}, {}};

static struct platform_driver m_stCamClkUtPlatformDriver = {
    .probe  = DrvCamClkUtModuleProbe,
    .remove = DrvCamClkUtModuleRemove,
    .driver =
        {
            .name           = DRV_CAMCLKUT_DEVICE_NAME,
            .owner          = THIS_MODULE,
            .of_match_table = of_match_ptr(m_stCamClkUtMatchTable),
        },
};

static struct platform_device m_stDrvCamClkUtPlatformDevice = {
    .name = DRV_CAMCLKUT_DEVICE_NAME, .id = 0, .dev = {.of_node = NULL, .coherent_dma_mask = 0xffffffffUL}};
static ssize_t Camclk_test_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t n)
{
    if (NULL != buf)
    {
        DrvCamclkUtCmdParser(buf, n);
        return n;
    }

    return 0;
}

static ssize_t Camclk_test_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return DrvCamclkUtShow(buf);
}

static DEVICE_ATTR(test, 0644, Camclk_test_show, Camclk_test_store);
void _DrvCamClkUtModuleInit(void)
{
    int   s32Ret = 0;
    dev_t dev;

    if (m_stCamClkUtDevice.refCnt == 0)
    {
        m_stCamClkUtDevice.refCnt++;

        CAMCLKINFO("[%s @ %d]\n", __FUNCTION__, __LINE__);
        s32Ret =
            alloc_chrdev_region(&dev, m_stCamClkUtDevice.s32Minor, DRV_CAMCLKUT_DEVICE_COUNT, DRV_CAMCLKUT_DEVICE_NAME);
        m_stCamClkUtDevice.s32Major = MAJOR(dev);

        // dev = MKDEV(m_stCamClkUtDevice.s32Major, m_stCamClkUtDevice.s32Minor);

        if (IS_ERR(m_pstCamClkUtClass))
        {
            CAMCLKERR("[%s @ %d] class_create() fail. \n", __FUNCTION__, __LINE__);
        }
        else
        {
            // Initialize a cdev structure
            cdev_init(&m_stCamClkUtDevice.cdev, &m_stCamClkUtDevice.fops);

            // Add a char device to the system
            if (0 != (s32Ret = cdev_add(&m_stCamClkUtDevice.cdev, dev, DRV_CAMCLKUT_DEVICE_COUNT)))
            {
                CAMCLKERR(" [%s @ %d] cdev_add() fail\n", __FUNCTION__, __LINE__);
            }
        }

        if (m_stCamClkUtDevice.devicenode == NULL && m_pstCamClkUtClass)
        {
            // Creates a device and registers it with sysfs
            m_stCamClkUtDevice.devicenode =
                CamDeviceCreate(m_pstCamClkUtClass, NULL, dev, NULL, DRV_CAMCLKUT_DEVICE_NAME);

            if (NULL == m_stCamClkUtDevice.devicenode)
            {
                CAMCLKERR("[%s @ %d] device_create() fail\n", __FUNCTION__, __LINE__);
            }
        }

        // Initialize platform device of_node
        if (m_stDrvCamClkUtPlatformDevice.dev.of_node == NULL)
        {
            m_stDrvCamClkUtPlatformDevice.dev.of_node = of_find_compatible_node(NULL, NULL, DRV_CAMCLKUT_DEVICE_NODE);
        }
        if (m_stDrvCamClkUtPlatformDevice.dev.of_node == NULL)
        {
            CAMCLKERR("[%s @ %d] Get device node fail\n", __FUNCTION__, __LINE__);
        }
        CamDeviceCreateFile((struct device *)m_stCamClkUtDevice.devicenode, &dev_attr_test);
    }
    else
    {
        m_stCamClkUtDevice.refCnt++;
    }
}

void _DrvCamClkUtModuleDeInit(void)
{
    if (m_stCamClkUtDevice.refCnt)
    {
        m_stCamClkUtDevice.refCnt--;
    }

    if (m_stCamClkUtDevice.refCnt == 0)
    {
        CAMCLKERR("[%s @ %d]\n", __FUNCTION__, __LINE__);
        if (m_stCamClkUtDevice.cdev.count)
        {
            // Remove a cdev from the system
            cdev_del(&m_stCamClkUtDevice.cdev);
        }
        m_stDrvCamClkUtPlatformDevice.dev.of_node = NULL;
        m_pstCamClkUtClass                        = NULL;
    }
}
//-------------------------------------------------------------------------------------------------
// Module functions
//-------------------------------------------------------------------------------------------------
static int DrvCamClkUtModuleProbe(struct platform_device *pdev)
{
    int   s32Ret = 0;
    dev_t dev;

    CAMCLKINFO("[%s @ %d]\n", __FUNCTION__, __LINE__);

    if (m_stCamClkUtDevice.s32Major == 0)
    {
        // Register a range of char device numbers
        s32Ret =
            alloc_chrdev_region(&dev, m_stCamClkUtDevice.s32Minor, DRV_CAMCLKUT_DEVICE_COUNT, DRV_CAMCLKUT_DEVICE_NAME);
        m_stCamClkUtDevice.s32Major = MAJOR(dev);
    }

    m_pstCamClkUtClass = msys_get_sysfs_class();

    if (IS_ERR(m_pstCamClkUtClass))
    {
        CAMCLKERR("[%s @ %d] msys_get_sysfs_class() fail. \n", __FUNCTION__, __LINE__);
    }

    m_stDrvCamClkUtPlatformDevice.dev.of_node = pdev->dev.of_node;

    // Create device
    _DrvCamClkUtModuleInit();
    return 0;
}

static int DrvCamClkUtModuleRemove(struct platform_device *pdev)
{
    CAMCLKINFO("[%s @ %d]\n", __FUNCTION__, __LINE__);
    _DrvCamClkUtModuleDeInit();
    CamDeviceUnregister(m_stCamClkUtDevice.devicenode);

    return 0;
}

int _MDrv_CamClkUt_ModuleInit(void)
{
    int ret = 0;
    CAMCLKDBG("[%s @ %d]\n", __FUNCTION__, __LINE__);

    // Register a driver for platform-level devices
    ret = CamPlatformDriverRegister(&m_stCamClkUtPlatformDriver);

    if (!ret)
    {
        CAMCLKDBG("[%s] CamPlatformDriverRegister() success\n", __FUNCTION__);
    }
    else
    {
        CAMCLKERR("[%s @ %d] CamPlatformDriverRegister() fail\n", __FUNCTION__, __LINE__);
        CamPlatformDriverUnregister(&m_stCamClkUtPlatformDriver);
    }

    return ret;
}

void _MDrv_CamClkUt_ModuleExit(void)
{
    CamPlatformDriverUnregister(&m_stCamClkUtPlatformDriver);
}
module_init(_MDrv_CamClkUt_ModuleInit);
module_exit(_MDrv_CamClkUt_ModuleExit);
MODULE_AUTHOR("CAMDRIVER");
MODULE_DESCRIPTION("CLK driver UT");
