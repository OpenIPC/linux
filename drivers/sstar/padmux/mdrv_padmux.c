/*
* mdrv_padmux.c- Sigmastar
*
* Copyright (C) 2018 Sigmastar Technology Corp.
*
* Author: richard.guo <richard.guo@sigmastar.com.tw>
*
* This software is licensed under the terms of the GNU General Public
* License version 2, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*/
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/slab.h>
#include "mdrv_types.h"

//-------------------------------------------------------------------------------------------------
//  Driver Compiler Options
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  Local Defines
//-------------------------------------------------------------------------------------------------
#define PADINFO_NAME    "schematic"

typedef struct
{
    U32         u32PadId;
    U32         u32Mode;
    U32         u32Puse;
} __attribute__ ((__packed__)) pad_info_t;

//-------------------------------------------------------------------------------------------------
//  Local Variables
//-------------------------------------------------------------------------------------------------
static int                      _nPad = 0;
static pad_info_t*              _pPadInfo = NULL;

//-------------------------------------------------------------------------------------------------
//  Implementation
//-------------------------------------------------------------------------------------------------
int mdrv_padmux_active(void)
{
    return (_pPadInfo) ? 1: 0;
}
EXPORT_SYMBOL(mdrv_padmux_active);

static int _mdrv_padmux_dts(struct device_node* np)
{
    int nPad;

    if (0 >= (nPad = of_property_count_elems_of_size(np, PADINFO_NAME, sizeof(pad_info_t))))
    {
        printk("[%s][%d] invalid dts of padmux.schematic\n", __FUNCTION__, __LINE__);
        return -1;
    }
    if (NULL == (_pPadInfo = kmalloc(nPad*sizeof(pad_info_t), GFP_KERNEL)))
    {
        printk("[%s][%d] kmalloc fail\n", __FUNCTION__, __LINE__);
        return -1;
    }
    if (of_property_read_u32_array(np, PADINFO_NAME, (u32*)_pPadInfo, nPad*sizeof(pad_info_t)/sizeof(U32)))
    {
        printk("[%s][%d] of_property_read_u32_array fail\n", __FUNCTION__, __LINE__);
        kfree(_pPadInfo);
        _pPadInfo = NULL;
        return -1;
    }
    _nPad = nPad;
#if 0
    {
        int i;
        printk("[%s][%d] *******************************\n", __FUNCTION__, __LINE__);
        for (i = 0; i < _nPad; i++)
        {
            printk("[%s][%d] (PadId, Mode, Puse) = (0x%08x, 0x%08x, 0x%08x)\n", __FUNCTION__, __LINE__,
                _pPadInfo[i].u32PadId, 
                _pPadInfo[i].u32Mode, 
                _pPadInfo[i].u32Puse);
        }
        printk("[%s][%d] *******************************\n", __FUNCTION__, __LINE__);
    }
#endif
    return 0;
}

static int sstar_padmux_probe(struct platform_device *pdev)
{
    _mdrv_padmux_dts(pdev->dev.of_node);
    return 0;
}

static const struct of_device_id sstar_padmux_of_match[] = {
    { .compatible = "sstar-padmux" },
    { },
};

static struct platform_driver sstar_padmux_driver = {
    .driver     = {
        .name   = "padmux",
        .owner  = THIS_MODULE,
        .of_match_table = sstar_padmux_of_match,
    },
    .probe      = sstar_padmux_probe,
};

static int __init sstar_padmux_init(void)
{
    return platform_driver_register(&sstar_padmux_driver);
}
postcore_initcall_sync(sstar_padmux_init);

MODULE_AUTHOR("SSTAR");
MODULE_DESCRIPTION("padmux driver");
MODULE_LICENSE("GPL");
