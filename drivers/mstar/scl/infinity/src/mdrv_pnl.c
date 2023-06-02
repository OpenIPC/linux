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
#define _MDRV_PNL_C
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include "MsCommon.h"
#include "MsTypes.h"
#include "MsOS.h"
#include "drvpnl.h"

#include "mdrv_pnl_io_st.h"
#include "mdrv_scl_dbg.h"
#include "mdrv_pnl.h"
//-------------------------------------------------------------------------------------------------
//  Defines & Macro
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  Variable
//-------------------------------------------------------------------------------------------------
/////////////////
/// gstCfg
/// use in suspend/resume to save information
////////////////
ST_MDRV_PNL_TIMING_CONFIG gstCfg;
/////////////////
/// gbResume
/// use to determine whether need to set timing config as resume.
////////////////
unsigned char gbResume=0;

//-------------------------------------------------------------------------------------------------
//  Function
//-------------------------------------------------------------------------------------------------
void MDrv_PNL_Release(void)
{
    if(gbResume)
    {
        Drv_PNL_Release();
    }
}
unsigned char MDrv_PNL_Init(ST_MDRV_PNL_INIT_CONFIG *pCfg)
{
    ST_PNL_INIT_CONFIG stInitCfg;
    stInitCfg.u32RiuBase = pCfg->u32RiuBase;

    if(Drv_PNL_Init(&stInitCfg) == 0)
    {
        SCL_ERR("[PNL]%s, Init Fail\n", __FUNCTION__);
        return 0;
    }
    else
    {
        return 1;
    }
}
void MDrv_PNL_Resume(void)
{
    Drv_PNL_Resume();
    if(gbResume)
    {
        MDrv_PNL_Set_Timing_Config(&gstCfg);
    }
}
unsigned char MDrv_PNL_Set_Timing_Config(ST_MDRV_PNL_TIMING_CONFIG *pCfg)
{
    ST_PNL_TIMING_CONFIG stTimingCfg;

    stTimingCfg.u16Vsync_St  = pCfg->u16Vsync_St;
    stTimingCfg.u16Vsync_End = pCfg->u16Vsync_End;
    stTimingCfg.u16Vde_St    = pCfg->u16Vde_St;
    stTimingCfg.u16Vde_End   = pCfg->u16Vde_End;
    stTimingCfg.u16Vfde_St   = pCfg->u16Vfde_St;
    stTimingCfg.u16Vfde_End  = pCfg->u16Vfde_End;
    stTimingCfg.u16Vtt       = pCfg->u16Vtt;

    stTimingCfg.u16Hsync_St  = pCfg->u16Hsync_St;
    stTimingCfg.u16Hsync_End = pCfg->u16Hsync_End;
    stTimingCfg.u16Hde_St    = pCfg->u16Hde_St;
    stTimingCfg.u16Hde_End   = pCfg->u16Hde_End;
    stTimingCfg.u16Hfde_St   = pCfg->u16Hfde_St;
    stTimingCfg.u16Hfde_End  = pCfg->u16Hfde_End;

    stTimingCfg.u16Htt       = pCfg->u16Htt;

    stTimingCfg.u16VFreqx10  = pCfg->u16VFreqx10;
    MsOS_Memcpy(&gstCfg, pCfg,sizeof(ST_MDRV_PNL_TIMING_CONFIG));
    gbResume=1;
    if( Drv_PNL_Set_Timing_Config(&stTimingCfg) == 0)
    {
        SCL_ERR("[PNL]%s, Set Timing Fail\n", __FUNCTION__);
        return 0;
    }
    else
    {
        return 1;
    }

}
