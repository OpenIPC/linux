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

#define  DRV_CAMCLK_C
#include "cam_os_wrapper.h"
#include "drv_camclk_st.h"
#include "drv_camclk.h"
#include "drv_camclk_Api.h"
#include "hal_camclk_if.h"
#include "camclk_dbg.h"
//==================================
#include "drv_dualos.h"

//======================================
//extern void recode_timestamp(int mark, const char* name);
#ifdef CONFIG_CAM_CLK_PROFILING
extern void BootTimestampRecord(int mark, const char* name);
void DrvCamClkOsProf(int mark, const char* name)
{
    BootTimestampRecord(mark,name);
}
#endif
#ifdef CONFIG_SS_DUALOS

u32 DrvCamClkOsRTKGetShareMemAddrCurrent(u32 arg0, u32 arg1, u32 arg2, u32 arg3)
{
    CAM_CLK_RECORD("RTKGetShareMemAddr+");
    CAMCLKDBG("[%s @ %d] ShareAddr:%p id:%lx\n", __FUNCTION__, __LINE__,gCamClkTopCurrent,arg0);
    if(arg0==CAM_CLK_INTEROS_SHAREMEM)
    {
        CAM_CLK_FLUSH((void*)gCamClkTopCurrent);
		CAM_CLK_RECORD("RTKGetShareMemAddr-");
        return (u32)gCamClkTopCurrent;
    }
    
    return 0;
}
#endif
void DrvCamClkOsPrepareShareMemory(DrvCamClkShareMemoryType_e enType)
{
#ifdef CONFIG_SS_DUALOS
    if(enType==DRV_CAMCLK_SHAREMEM_TOPCURRENT)
    {
        interos_sc_reg(CAM_CLK_INTEROS_SHAREMEM, DrvCamClkOsRTKGetShareMemAddrCurrent, "SHARECURRENT");
    }
#endif
}
