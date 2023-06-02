/* Copyright (c) 2018-2019 Sigmastar Technology Corp.
 All rights reserved.

 Unless otherwise stipulated in writing, any and all information contained
herein regardless in any format shall remain the sole proprietary of
Sigmastar Technology Corp. and be kept in strict confidence
(Sigmastar Confidential Information) by the recipient.
Any unauthorized act including without limitation unauthorized disclosure,
copying, use, reproduction, sale, distribution, modification, disassembling,
reverse engineering and compiling of the contents of Sigmastar Confidential
Information is unlawful and strictly prohibited. Sigmastar hereby reserves the
rights to any and all damages, losses, costs and expenses resulting therefrom.
*/

#define __CAMCLK_SYSFS_C__

//-------------------------------------------------------------------------------------------------
// Include files
//-------------------------------------------------------------------------------------------------

#include <linux/device.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>

#include "cam_sysfs.h"
#include "cam_clkgen.h"
#include "cam_sysfs.h"
#include "cam_os_wrapper.h"
#include "drv_camclk_Api.h"
#include "drv_camclk_DataType.h"
#include "drv_camclk.h"
#include "camclk_dbg.h"
#include "drv_camclk.h"

//-------------------------------------------------------------------------------------------------
// Variable
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
// Internal function
//-------------------------------------------------------------------------------------------------

#ifdef CONFIG_CAM_CLK_SYSFS

static ssize_t Camclk_handlerinfo_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t n)
{
    if (NULL != buf)
    {


        return n;
    }

    return 0;
}

static ssize_t Camclk_handlerinfo_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return DrvCamclkDebugHandlerShow(buf);
}

static DEVICE_ATTR(handlerinfo, 0644, Camclk_handlerinfo_show, Camclk_handlerinfo_store);


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static ssize_t Camclk_debuglvl_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t n)
{
    if (NULL != buf)
    {
        DrvCamclkDebugDebugLvlStore(buf);
        return n;
    }

    return 0;
}

static ssize_t Camclk_debuglvl_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return DrvCamclkDebugDebugLvlShow(buf);
}

static DEVICE_ATTR(debuglvl, 0644, Camclk_debuglvl_show, Camclk_debuglvl_store);


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static ssize_t Camclk_clkinfo_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t n)
{
    if (NULL != buf)
    {
        return n;
    }

    return 0;
}

static ssize_t Camclk_clkinfo_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return DrvCamclkDebugClkShow(buf);
}

static DEVICE_ATTR(clkinfo, 0644, Camclk_clkinfo_show, Camclk_clkinfo_store);
#endif
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void DrvCamClkSysfsInit(void *device)
{
#ifdef CONFIG_CAM_CLK_SYSFS
    DrvCamClkDebugInit();

    // Create device attribute
    if(CamDeviceCreateFile((struct device *)device, &dev_attr_handlerinfo))
    {
        CAMCLKERR("[%s @ %d] Sysfs Create Fail.\n", __FUNCTION__, __LINE__);
    }
    CamDeviceCreateFile((struct device *)device, &dev_attr_debuglvl);
    CamDeviceCreateFile((struct device *)device, &dev_attr_clkinfo);
#endif
}

#undef __CAMCLK_SYSFS_C__
