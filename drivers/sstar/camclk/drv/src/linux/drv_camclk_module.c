/*
 * drv_camclk_module.c- Sigmastar
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

#define DRV_CAMCLK_DEVICE_COUNT    1
#define DRV_CAMCLK_DEVICE_NAME     "camclk"
#define DRV_CAMCLK_DEVICE_MAJOR    0x8a
#define DRV_CAMCLK_DEVICE_MINOR    0x0
#define DRV_CAMCLK_DEVICE_NODE     "camdriver,camclk"
#define DRV_CAMCLK_DEVICEINIT_NODE "camdriver,camclkinit"
//-------------------------------------------------------------------------------------------------
// Prototype
//-------------------------------------------------------------------------------------------------

static int DrvCamClkModuleProbe(struct platform_device *pdev);
static int DrvCamClkModuleRemove(struct platform_device *pdev);
static int DrvCamClkModuleSuspend(struct platform_device *pdev, pm_message_t state);
static int DrvCamClkModuleResume(struct platform_device *pdev);

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

} DrvCamClkModuleDevice_t;

//-------------------------------------------------------------------------------------------------
// Variable
//-------------------------------------------------------------------------------------------------

static DrvCamClkModuleDevice_t m_stCamClkDevice = {
    .s32Major   = 0,
    .s32Minor   = DRV_CAMCLK_DEVICE_MINOR,
    .refCnt     = 0,
    .binit      = 0,
    .devicenode = NULL,
    .cdev =
        {
            .kobj =
                {
                    .name = DRV_CAMCLK_DEVICE_NAME,
                },
            .owner = THIS_MODULE,
        },
};

static struct class *m_pstCamClkClass = NULL;

static const struct of_device_id m_stCamClkMatchTable[] = {{.compatible = DRV_CAMCLK_DEVICE_NODE}, {}};

static struct platform_driver m_stCamClkPlatformDriver = {
    .probe   = DrvCamClkModuleProbe,
    .remove  = DrvCamClkModuleRemove,
    .suspend = DrvCamClkModuleSuspend,
    .resume  = DrvCamClkModuleResume,
    .driver =
        {
            .name           = DRV_CAMCLK_DEVICE_NAME,
            .owner          = THIS_MODULE,
            .of_match_table = of_match_ptr(m_stCamClkMatchTable),
        },
};

static struct platform_device m_stDrvCamClkPlatformDevice = {
    .name = DRV_CAMCLK_DEVICE_NAME, .id = 0, .dev = {.of_node = NULL, .coherent_dma_mask = 0xffffffffUL}};

void _DrvCamClkModuleInit(void)
{
#ifdef CONFIG_CAM_CLK_SYSFS
    dev_t dev = 0;
#endif
    if (m_stCamClkDevice.refCnt == 0)
    {
        m_stCamClkDevice.refCnt++;
#ifdef CONFIG_CAM_CLK_SYSFS
        m_pstCamClkClass = msys_get_sysfs_class();

        if (IS_ERR(m_pstCamClkClass))
        {
            CAMCLKERR("[%s @ %d] msys_get_sysfs_class() fail. \n", __FUNCTION__, __LINE__);
        }

        if (m_stCamClkDevice.devicenode == NULL && m_pstCamClkClass)
        {
            // Creates a device and registers it with sysfs
            m_stCamClkDevice.devicenode = CamDeviceCreate(m_pstCamClkClass, NULL, dev, NULL, DRV_CAMCLK_DEVICE_NAME);

            if (NULL == m_stCamClkDevice.devicenode)
            {
                CAMCLKERR("[%s @ %d] device_create() fail\n", __FUNCTION__, __LINE__);
            }
        }
#endif
        if (m_stCamClkDevice.binit == 0)
        {
            CamClkInit();
        }
        DrvCamClkOsGetShareMemory(DRV_CAMCLK_SHAREMEM_TOPCURRENT);
        DrvCamClkSysfsInit(m_stCamClkDevice.devicenode);
    }
    else
    {
        m_stCamClkDevice.refCnt++;
    }
}

void _DrvCamClkModuleDeInit(void)
{
    if (m_stCamClkDevice.refCnt)
    {
        m_stCamClkDevice.refCnt--;
    }

    if (m_stCamClkDevice.refCnt == 0)
    {
        CAMCLKERR("[%s @ %d]\n", __FUNCTION__, __LINE__);
        DrvCamClkOsPutShareMemory(DRV_CAMCLK_SHAREMEM_TOPCURRENT);
        CamClkDeinit();
        m_stDrvCamClkPlatformDevice.dev.of_node = NULL;
        m_pstCamClkClass                        = NULL;
    }
}
//-------------------------------------------------------------------------------------------------
// Module functions
//-------------------------------------------------------------------------------------------------
static int DrvCamClkModuleProbe(struct platform_device *pdev)
{
    CAMCLKINFO("[%s @ %d]\n", __FUNCTION__, __LINE__);
    CAM_CLK_RECORD("CamClkProbe+");
    // Create device
    m_stDrvCamClkPlatformDevice.dev.of_node = pdev->dev.of_node;
    _DrvCamClkModuleInit();
    CAM_CLK_RECORD("CamClkProbe-");
    return 0;
}

static int DrvCamClkModuleRemove(struct platform_device *pdev)
{
    CAMCLKINFO("[%s @ %d]\n", __FUNCTION__, __LINE__);
    _DrvCamClkModuleDeInit();
    CamDeviceUnregister(m_stCamClkDevice.devicenode);

    return 0;
}

static int DrvCamClkModuleSuspend(struct platform_device *dev, pm_message_t state)
{
    CAMCLKDBG("[%s @ %d]\n", __FUNCTION__, __LINE__);

    return 0;
}

static int DrvCamClkModuleResume(struct platform_device *dev)
{
    CAMCLKDBG("[%s @ %d]\n", __FUNCTION__, __LINE__);
    DrvCamClkResume();
    return 0;
}
/*
int _MDrv_CamClk_ModuleInit(void)
{
    int ret = 0;
    //CAM_CLK_PROFILE_INIT();
    CAM_CLK_RECORD("ClkInit+");
    CAMCLKDBG("[%s @ %d]\n", __FUNCTION__, __LINE__);

    // Register a driver for platform-level devices
    ret = CamPlatformDriverRegister(&m_stCamClkPlatformDriver);

    if (!ret)
    {
        CAMCLKDBG("[%s] CamPlatformDriverRegister() success\n", __FUNCTION__);
    }
    else
    {
        CAMCLKERR("[%s @ %d] CamPlatformDriverRegister() fail\n", __FUNCTION__, __LINE__);
        CamPlatformDriverUnregister(&m_stCamClkPlatformDriver);
    }
    //CAM_CLK_PROFILE_DONE();
    CAM_CLK_RECORD("ClkInit-");
    return ret;
}


void _MDrv_CamClk_ModuleExit(void)
{
    CamPlatformDriverUnregister(&m_stCamClkPlatformDriver);
}
*/
void __init CamClk_init(struct device_node *node)
{
    CAM_CLK_RECORD("ClkPreInit+");
    CAMCLKDBG("[%s @ %d]\n", __FUNCTION__, __LINE__);
    CamClkInit();
    m_stCamClkDevice.binit = 1;
    CAM_CLK_RECORD("ClkPreInit-");
}
builtin_platform_driver(m_stCamClkPlatformDriver);
// early_initcall(_MDrv_CamClk_ModuleInit);
// fs_initcall_sync(_MDrv_CamClk_ModuleInit);
CLK_OF_DECLARE(CamClk, DRV_CAMCLK_DEVICEINIT_NODE, CamClk_init);
// module_init(_MDrv_CamClk_ModuleInit);
// module_exit(_MDrv_CamClk_ModuleExit);
late_initcall_sync(DrvCamClkImplDisableUnuseClk);
// CLK_OF_DECLARE(CamClk,DRV_CAMCLK_DEVICE_NODE,DrvCamClkInit);
// CLK_OF_DECLARE(ms_clk_composite, "sstar,composite-clock", ms_clk_composite_init);
MODULE_AUTHOR("CAMDRIVER");
MODULE_DESCRIPTION("CLK driver");
EXPORT_SYMBOL(CamClkAttrGet);
EXPORT_SYMBOL(CamClkAttrSet);
EXPORT_SYMBOL(CamClkSetOnOff);
EXPORT_SYMBOL(CamClkGetOnOff);
EXPORT_SYMBOL(CamClkRegister);
EXPORT_SYMBOL(CamClkUnregister);
EXPORT_SYMBOL(CamClkRateGet);
EXPORT_SYMBOL(gCAMCLKDbgLvl);
