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

#ifndef MSTAR_MCI_H
#define MSTAR_MCI_H

#include "eMMC.h"

/******************************************************************************
* Function define for this driver
******************************************************************************/

/******************************************************************************
* Register Address Base
******************************************************************************/
#define CLK_400KHz       		400*1000
#define CLK_200MHz       		200*1000*1000

#define eMMC_GENERIC_WAIT_TIME  (HW_TIMER_DELAY_1s*10)
#define eMMC_READ_WAIT_TIME     (HW_TIMER_DELAY_500ms)

/******************************************************************************
* Low level type for this driver
******************************************************************************/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,0,20)
#if defined(ENABLE_EMMC_ASYNC_IO) && ENABLE_EMMC_ASYNC_IO
struct mstar_mci_host_next
{
    unsigned int                dma_len;
    s32                         cookie;
};
#endif
#endif

struct mstar_mci_host
{
    struct mmc_host			    *mmc;
    struct mmc_command		    *cmd;
    struct mmc_request		    *request;

    void __iomem			    *baseaddr;
    s32						    irq;

    u16						    sd_clk;
    u16						    sd_mod;

    #if LINUX_VERSION_CODE >= KERNEL_VERSION(3,0,20)
    #if defined(ENABLE_EMMC_ASYNC_IO) && ENABLE_EMMC_ASYNC_IO
    struct mstar_mci_host_next  next_data;
    struct work_struct          async_work;
    #endif
    #endif
    char                    name[16];	
};  /* struct mstar_mci_host*/

#define MSTAR_MCI_NAME "MSTAR_MCI"

#endif
