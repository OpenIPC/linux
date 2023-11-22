/*
 * drv_camclk_os.c- Sigmastar
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

#define DRV_CAMCLK_C
#include "cam_os_wrapper.h"
#include "drv_camclk_st.h"
#include "drv_camclk.h"
#include "drv_camclk_Api.h"
#include "hal_camclk_if.h"
#include "camclk_dbg.h"
//==================================
#include "drv_dualos.h"
#include "cam_inter_os.h"
//======================================

#ifdef CONFIG_CAM_CLK_PROFILING
extern void BootTimestampRecord(int mark, const char* name);
void        DrvCamClkOsProf(int mark, const char* name)
{
    BootTimestampRecord(mark, name);
}
#endif
#ifdef CONFIG_SS_DUALOS

u32 DrvCamClkOsRTKGetShareMemAddrCurrent(u32 arg0, u32 arg1, u32 arg2, u32 arg3)
{
    DrvCamClkGetDosInfo_t* pstDosInfo;
    CAM_CLK_RECORD("RTKGetShareMemAddr+");
    CAMCLKDBG("[%s @ %d] ShareAddr:%p id:%lx LockNum:%lx\n", __FUNCTION__, __LINE__, gCamClkTopCurrent, arg0,
              gu32CamclkOsLockNum);
    if (arg0 == CAM_CLK_INTEROS_SHAREMEM)
    {
        if (arg1)
        {
            pstDosInfo             = (DrvCamClkGetDosInfo_t*)arg1;
            pstDosInfo->u32LockNum = gu32CamclkOsLockNum;
            pstDosInfo->pMem       = gCamClkTopCurrent;
            CAM_CLK_FLUSH((void*)pstDosInfo);
        }
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
    if (enType == DRV_CAMCLK_SHAREMEM_TOPCURRENT)
    {
        CamInterOsSignalReg(CAM_CLK_INTEROS_SHAREMEM, DrvCamClkOsRTKGetShareMemAddrCurrent, "SHARECURRENT");
    }
#endif
}
void DrvCamClkOsDestroyShareMemory(DrvCamClkShareMemoryType_e enType)
{
#ifdef CONFIG_SS_DUALOS
    if (enType == DRV_CAMCLK_SHAREMEM_TOPCURRENT)
    {
        CamInterOsSignalDereg(CAM_CLK_INTEROS_SHAREMEM, DrvCamClkOsRTKGetShareMemAddrCurrent);
    }
#endif
}
