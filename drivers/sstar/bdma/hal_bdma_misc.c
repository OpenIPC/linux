/*
 * hal_bdma_misc.c- Sigmastar
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

// Include files
/*=============================================================*/

#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/slab.h>
//#include "ms_platform.h"
//#include "ms_types.h"
//#include "registers.h"
#include "hal_bdma.h"
#include "cam_os_wrapper.h"
#ifdef CONFIG_CAM_CLK
#include "camclk.h"
#include "drv_camclk_Api.h"
#endif

extern bool m_bBdmaInited[HAL_BDMA_CH_NUM];

//------------------------------------------------------------------------------
//  Function    : _HalBdma_EnClkOnce
//  Description :
//------------------------------------------------------------------------------
void _HalBdma_EnClkOnce(void *pdev_node)
{
    struct device_node *dev_node = (struct device_node *)pdev_node;
#ifdef CONFIG_CAM_CLK
    u32   u32clknum = 0, i;
    u32   BdmaClk;
    void *pvBdmaclk     = NULL;
    u32   BdmaParentCnt = 1;
#else
    struct clk **bdma_clks;
    int          num_parents, i;
#endif

    // If any channel of BDMA is initialized, the power is already turn on
    for (i = 0; i < HAL_BDMA_CH_NUM; i++)
    {
        if (m_bBdmaInited[i])
            return;
    }
#ifdef CONFIG_CAM_CLK
    if (of_find_property(dev_node, "camclk", &BdmaParentCnt))
    {
        BdmaParentCnt /= sizeof(int);
        // printk( "[%s] Number : %d\n", __func__, num_parents);
        if (BdmaParentCnt < 0)
        {
            printk("[%s] Fail to get parent count! Error Number : %d\n", __func__, BdmaParentCnt);
            return;
        }
        for (u32clknum = 0; u32clknum < BdmaParentCnt; u32clknum++)
        {
            BdmaClk = 0;
            of_property_read_u32_index(dev_node, "camclk", u32clknum, &(BdmaClk));
            if (!BdmaClk)
            {
                printk(KERN_DEBUG "[%s] Fail to get clk!\n", __func__);
            }
            else
            {
                CamClkRegister("BDMA", BdmaClk, &(pvBdmaclk));
                CamClkSetOnOff(pvBdmaclk, 1);
                CamClkUnregister(pvBdmaclk);
            }
        }
    }
    else
    {
        printk("[%s] W/O Camclk \n", __func__);
    }
#else
#if CONFIG_OF
    num_parents = of_clk_get_parent_count(dev_node);
    if (num_parents > 0)
    {
        bdma_clks = kzalloc((sizeof(struct clk *) * num_parents), GFP_KERNEL);
        if (bdma_clks == NULL)
        {
            printk("[BDMA]kzalloc failed!\n");
            return;
        }

        // enable all clk
        for (i = 0; i < num_parents; i++)
        {
            bdma_clks[i] = of_clk_get(dev_node, i);
            if (IS_ERR(bdma_clks[i]))
            {
                printk("Fail to get BDMA clk!\n");
                clk_put(bdma_clks[i]);
                kfree(bdma_clks);
                return;
            }
            else
            {
                /* Set clock parent */
                clk_prepare_enable(bdma_clks[i]);
                clk_put(bdma_clks[i]);
            }
        }
        kfree(bdma_clks);
    }
#endif
#endif
}

HalBdmaErr_e _HalBdma_RequestIRQ(void *pdev_node, void *phandler)
{
    struct device_node *dev_node     = (struct device_node *)pdev_node;
    irq_handler_t       pfIrqHandler = (irq_handler_t)phandler;
    int                 iIrqNum      = 0;

    /* Register interrupt handler */
    iIrqNum = irq_of_parse_and_map(dev_node, 0);

    if (0 != request_irq(iIrqNum, pfIrqHandler, 0, "BdmaIsr", NULL))
    {
        CamOsPrintf("[BDMA] request_irq [%d] Fail\r\n", iIrqNum);
        return HAL_BDMA_ERROR;
    }
    else
    {
        // CamOsPrintf("[BDMA] request_irq [%d] OK\r\n", iIrqNum);
    }

    return HAL_BDMA_PROC_DONE;
}
